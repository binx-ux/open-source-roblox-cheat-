#include "scanner.h"
#include "../memory/memory.h"
#include "offsets.h"
#include "sdk.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include <windows.h>

namespace
{
	constexpr std::size_t kMaxChunk = 1u * 1024u * 1024u; // 1 MB chunks

	bool looks_like_user_ptr(std::uint64_t value)
	{
		return value >= 0x10000ULL && value <= 0x00007FFFFFFFFFFFULL && (value & 0x7) == 0;
	}

	bool is_finite_float(float value)
	{
		return std::isfinite(value);
	}

	bool class_is_data_model(std::uint64_t instance)
	{
		if (!looks_like_user_ptr(instance))
			return false;

		const auto classDesc = memory->read<std::uint64_t>(instance + Offsets::Instance::ClassDescriptor);
		if (!looks_like_user_ptr(classDesc))
			return false;

		const auto namePtr = memory->read<std::uint64_t>(classDesc + Offsets::Instance::ClassName);
		if (!looks_like_user_ptr(namePtr))
			return false;

		return memory->read_string(namePtr) == "DataModel";
	}

	bool validate_data_model(std::uint64_t dataModel)
	{
		if (!class_is_data_model(dataModel))
			return false;

		// Only do the expensive child walk after class name matches.
		RBX::RbxInstance dm(dataModel);
		const auto workspace = dm.FindChildByClass("Workspace");
		const auto players = dm.FindChildByClass("Players");
		return workspace.Addr != 0 && players.Addr != 0;
	}

	bool validate_visual_engine(std::uint64_t visualEngine)
	{
		if (!looks_like_user_ptr(visualEngine))
			return false;

		RBX::Mat4 matrix{};
		try
		{
			matrix = memory->read<RBX::Mat4>(visualEngine + Offsets::VisualEngine::ViewMatrix);
		}
		catch (...)
		{
			return false;
		}

		int nonZero = 0;
		for (float value : matrix.data)
		{
			if (!is_finite_float(value))
				return false;
			if (std::fabs(value) > 0.00001f)
				++nonZero;
		}

		if (nonZero < 4)
			return false;

		const float width = memory->read<float>(visualEngine + Offsets::VisualEngine::Dimensions);
		const float height = memory->read<float>(visualEngine + Offsets::VisualEngine::Dimensions + 0x4);
		if (!is_finite_float(width) || !is_finite_float(height))
			return false;

		if (width == 0.0f && height == 0.0f)
			return true;

		return width >= 100.0f && width <= 16000.0f && height >= 100.0f && height <= 16000.0f;
	}

	bool try_resolve_from_fake(std::uint64_t fakeDataModel, std::uint64_t realOffset, std::uint64_t& outDataModel)
	{
		outDataModel = 0;
		if (!looks_like_user_ptr(fakeDataModel))
			return false;

		const auto dataModel = memory->read<std::uint64_t>(fakeDataModel + realOffset);
		if (!validate_data_model(dataModel))
			return false;

		outDataModel = dataModel;
		return true;
	}

	bool try_hardcoded(Scanner::AnchorResult& result)
	{
		const auto base = memory->get_module_address();
		if (!base)
			return false;

		const auto fakeStatic = base + Offsets::FakeDataModel::Pointer;
		const auto fakeDataModel = memory->read<std::uint64_t>(fakeStatic);
		std::uint64_t dataModel = 0;
		if (!try_resolve_from_fake(fakeDataModel, Offsets::FakeDataModel::RealDataModel, dataModel))
			return false;

		const auto visualStatic = base + Offsets::VisualEngine::Pointer;
		const auto visualEngine = memory->read<std::uint64_t>(visualStatic);
		if (!validate_visual_engine(visualEngine))
			return false;

		result.dataModel = dataModel;
		result.visualEngine = visualEngine;
		result.fakeDataModel = fakeDataModel;
		result.fakeDataModelRva = Offsets::FakeDataModel::Pointer;
		result.visualEngineRva = Offsets::VisualEngine::Pointer;
		result.realDataModelOffset = Offsets::FakeDataModel::RealDataModel;
		result.method = "offsets";
		result.success = true;
		return true;
	}

	void scan_region_chunk(
		std::uint64_t base,
		std::uint64_t scanStart,
		const std::uint8_t* data,
		std::size_t usable,
		const std::array<std::uint64_t, 6>& realOffsets,
		std::uint64_t& foundFakeRva,
		std::uint64_t& foundFake,
		std::uint64_t& foundDataModel,
		std::uint64_t& foundRealOffset,
		std::uint64_t& foundVisualRva,
		std::uint64_t& foundVisual)
	{
		for (std::size_t i = 0; i + 8 <= usable; i += 8)
		{
			std::uint64_t candidate = 0;
			std::memcpy(&candidate, data + i, sizeof(candidate));
			if (!looks_like_user_ptr(candidate))
				continue;

			const std::uint64_t rva = (scanStart + i) - base;

			if (!foundDataModel)
			{
				if (class_is_data_model(candidate) && validate_data_model(candidate))
				{
					foundFakeRva = rva;
					foundFake = candidate;
					foundDataModel = candidate;
					foundRealOffset = 0;
					std::cout << "[+] DataModel pointer RVA 0x" << std::hex << rva
						<< " DataModel 0x" << candidate << std::dec << "\n";
				}
				else
				{
					for (const auto realOffset : realOffsets)
					{
						std::uint64_t dataModel = 0;
						if (!try_resolve_from_fake(candidate, realOffset, dataModel))
							continue;

						foundFakeRva = rva;
						foundFake = candidate;
						foundDataModel = dataModel;
						foundRealOffset = realOffset;
						std::cout << "[+] FakeDataModel RVA 0x" << std::hex << rva
							<< " RealDataModel 0x" << realOffset
							<< " DataModel 0x" << dataModel << std::dec << "\n";
						break;
					}
				}
			}

			if (!foundVisual && validate_visual_engine(candidate))
			{
				foundVisualRva = rva;
				foundVisual = candidate;
				std::cout << "[+] VisualEngine RVA 0x" << std::hex << rva
					<< " VE 0x" << candidate << std::dec << "\n";
			}

			if (foundDataModel && foundVisual)
				return;
		}
	}

	bool scan_module_for_anchors(Scanner::AnchorResult& result)
	{
		const auto base = memory->get_module_address();
		const auto size = memory->get_module_size();
		const auto handle = memory->get_process_handle();
		if (!base || !size || !handle)
			return false;

		std::array<std::uint64_t, 6> realOffsets{};
		realOffsets[0] = Offsets::FakeDataModel::RealDataModel;
		realOffsets[1] = 0x1D0;
		realOffsets[2] = 0x1C0;
		realOffsets[3] = 0x1C8;
		realOffsets[4] = 0x1B8;
		realOffsets[5] = 0x1B0;

		std::cout << "[*] offset path failed - scanning module for anchors...\n";
		std::cout << "[*] module size: 0x" << std::hex << size << std::dec << "\n";

		std::uint64_t foundFakeRva = 0;
		std::uint64_t foundFake = 0;
		std::uint64_t foundDataModel = 0;
		std::uint64_t foundRealOffset = 0;
		std::uint64_t foundVisualRva = 0;
		std::uint64_t foundVisual = 0;

		MEMORY_BASIC_INFORMATION mbi{};
		std::uint64_t address = base;
		const std::uint64_t end = base + size;
		std::size_t regionsChecked = 0;

		std::vector<std::uint8_t> buffer(kMaxChunk);

		while (address < end)
		{
			if (VirtualQueryEx(handle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0)
				break;

			const auto regionBase = reinterpret_cast<std::uint64_t>(mbi.BaseAddress);
			const auto regionSize = static_cast<std::uint64_t>(mbi.RegionSize);
			const auto next = regionBase + regionSize;

			// Static pointers live in writable data, not code.
			const bool scannable =
				mbi.State == MEM_COMMIT &&
				!(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
				(mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));

			if (scannable && regionSize >= sizeof(std::uint64_t))
			{
				const std::uint64_t scanStart = (std::max)(regionBase, base);
				const std::uint64_t scanEnd = (std::min)(next, end);

				for (std::uint64_t chunkStart = scanStart; chunkStart < scanEnd; )
				{
					const std::uint64_t remaining = scanEnd - chunkStart;
					const std::size_t chunkSize = static_cast<std::size_t>((std::min<std::uint64_t>)(remaining, kMaxChunk));
					SIZE_T bytesRead = 0;

					if (!ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(chunkStart), buffer.data(), chunkSize, &bytesRead) || bytesRead < 8)
					{
						chunkStart += chunkSize;
						continue;
					}

					++regionsChecked;
					const std::size_t usable = bytesRead - (bytesRead % 8);
					scan_region_chunk(
						base,
						chunkStart,
						buffer.data(),
						usable,
						realOffsets,
						foundFakeRva,
						foundFake,
						foundDataModel,
						foundRealOffset,
						foundVisualRva,
						foundVisual);

					if (foundDataModel && foundVisual)
						break;

					chunkStart += chunkSize;
				}
			}

			if (foundDataModel && foundVisual)
				break;

			if (next <= address)
				break;
			address = next;
		}

		std::cout << "[*] chunks scanned: " << regionsChecked << "\n";

		if (!foundDataModel || !foundVisual)
			return false;

		result.dataModel = foundDataModel;
		result.visualEngine = foundVisual;
		result.fakeDataModel = foundFake;
		result.fakeDataModelRva = foundFakeRva;
		result.visualEngineRva = foundVisualRva;
		result.realDataModelOffset = foundRealOffset;
		result.method = "scan";
		result.success = true;
		return true;
	}
}

namespace Scanner
{
	AnchorResult ResolveAnchors()
	{
		AnchorResult result{};

		try
		{
			if (try_hardcoded(result))
			{
				std::cout << "[+] anchors resolved via offsets\n";
				return result;
			}

			if (scan_module_for_anchors(result))
			{
				std::cout << "[+] anchors resolved via module scan\n";
				return result;
			}
		}
		catch (const std::exception& ex)
		{
			std::cout << "[!] scanner exception: " << ex.what() << "\n";
			result.success = false;
			return result;
		}
		catch (...)
		{
			std::cout << "[!] scanner crashed on a bad read - aborting scan\n";
			result.success = false;
			return result;
		}

		std::cout << "[!] failed to resolve DataModel / VisualEngine\n";
		return result;
	}
}
