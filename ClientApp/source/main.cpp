//  Filename: main.cpp
//	Author:	Daniel														
//	Date: 17/04/2025 21:00:06		
//  Sqwack-Studios													

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

//Windows stuff
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d12.h>

//STD lib
#include <execution>
#include <iostream>
#include <chrono>


using namespace Microsoft::WRL;

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

//Engine
#include "RadiantEngine/core/types.h"
#include "RadiantEngine/math/floatN.h"
#include "D3D12MemAlloc.h"

using namespace RE;

internal constexpr fp32 ASPECT_RATIO{ 16.f / 9.f };
internal constexpr int16 INITIAL_HEIGHT{ 1080 };
internal constexpr int16 INITIAL_WIDTH{ static_cast<int16>(INITIAL_HEIGHT * ASPECT_RATIO) };
internal constexpr int8 NUM_FRAMES{ 3 };
internal constexpr wchar_t WINDOW_NAME[]{ L"FedeGordo" };



internal ComPtr<ID3D12Debug> debugLayer;
internal ComPtr<IDXGIDebug1> dxgidebug;
internal ComPtr<IDXGIFactory4> factory;
internal ComPtr<IDXGIAdapter1> adapter;
internal ComPtr<ID3D12Device> device;
internal ComPtr<IDXGISwapChain3> swapChain;

internal ComPtr<ID3D12CommandAllocator> commandAllocators[NUM_FRAMES];
internal ComPtr<ID3D12Resource> backBuffers[NUM_FRAMES];
internal ComPtr<ID3D12CommandQueue> directQueue;
internal ComPtr<ID3D12GraphicsCommandList> directList;
internal ComPtr<ID3D12Fence> directFence;

internal ComPtr<ID3D12RootSignature> rootSignature;
internal ComPtr<ID3D12PipelineState> pso;



internal uint64 frameDirectFenceValue[NUM_FRAMES];
internal HANDLE directFenceEvent;

internal uint8 currentFrame;
internal uint32 rtvDescriptorSize;
internal bool isFullscreen{ false };
internal RECT windowRect;

internal ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;

//D3D12MA stuff
D3D12MA::Allocator* d3dmallocator;

D3D12MA::Allocation* vtxStagingAllocation;
ComPtr<ID3D12Resource> vtxStagingBuffer;

D3D12MA::Allocation* vtxResidentAllocation;
ComPtr<ID3D12Resource> vtxResidentBuffer;
internal D3D12_VERTEX_BUFFER_VIEW vtxBufferView;

internal constexpr float4 red{ 1.0f, 0.0f, 0.0f, 1.0f };
internal constexpr float4 green{ 0.f, 1.0f, 0.0f, 1.0f };
internal constexpr float4 blue{ 0.f, 0.f, 1.0f, 1.0f };
internal constexpr float4 black{ 0.0f, 0.0f, 0.0f, 1.0f };

internal float4 clearColors[NUM_FRAMES]{ red, green, blue };

template<typename T>
struct Span
{
	T* data;
	size_t num;
};


template<typename T>
struct str
{
	T* data;
	size_t num;
	size_t cap;

	operator Span<T>()
	{
		return Span<T>{.data = data, .num = num };
	}

	template<typename T>
	operator Span<const T>()
	{
		return Span<const T>{.data = data, .num = num };
	}
};


template<typename T>
internal constexpr Span<T> ShaderRegistryPath();

namespace {

	constexpr const char shaderAssetsRelPath[]{ "../../shaders" };
	constexpr const wchar_t shaderAssetsRelWPath[]{ L"../../shaders" };
}

template<>
internal constexpr Span<const char> ShaderRegistryPath()
{
	return { .data = shaderAssetsRelPath, .num = _countof(shaderAssetsRelPath) };
}

template<>
internal constexpr Span<const wchar_t> ShaderRegistryPath()
{
	return {.data = shaderAssetsRelWPath, .num = _countof(shaderAssetsRelWPath) };
}


internal void WaitForFence(ID3D12Fence* fence, uint64 valueToWaitFor)
{
	uint64 fenceValue{ fence->GetCompletedValue() };
	if (fenceValue < valueToWaitFor)
	{
		fence->SetEventOnCompletion(valueToWaitFor, directFenceEvent);
		::WaitForSingleObjectEx(directFenceEvent, INFINITE, FALSE);
	}

}

internal void Flush(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64& fenceValue)
{
	queue->Signal(fence, ++fenceValue);
	WaitForFence(fence, fenceValue);
}


internal void Update(fp64 dt)
{

}

internal void PrepInitialDataUpload()
{
	//1- Read shaders blob - OK -
	//2- Serialize RootSignature - OK -
	//3- IA layout - OK -
	//4- PSO - OK -
	//5- Buffers (?) - OK - 
	//6- fence buffers upload
	//7- draw call


	static constexpr size_t _100kb{ 100000 };

	
	

	char vsBlob[_100kb];
	size_t vsByteSize;

	char psBlob[_100kb];
	size_t psByteSize;

	Span<const char> shadersRegistryPath{ ShaderRegistryPath<const char>() };

	{//Read VS
		char vsPathBuffer[128];

		str<char> vsShaderPath{ .data = vsPathBuffer,
		.num = shadersRegistryPath.num,
		.cap = 128 };

		char vsRegistryPath[]{ "/basicVS.cso" };
		FILE* vsFile{};

		strcpy(vsShaderPath.data, shadersRegistryPath.data);
		strncat(vsShaderPath.data, vsRegistryPath, _countof(vsRegistryPath));
		vsShaderPath.num += _countof(vsRegistryPath);

		int result{ fopen_s(&vsFile, vsShaderPath.data, "rb")};

		fseek(vsFile, 0, SEEK_END);
		vsByteSize = ftell(vsFile);
		rewind(vsFile);

		fread(vsBlob, vsByteSize, 1, vsFile);
		fclose(vsFile);

	}

	{//Read VS
		char psPathBuffer[128];

		str<char> psShaderPath{ .data = psPathBuffer,
		.num = shadersRegistryPath.num,
		.cap = 128 };

		char psRegistryPath[]{ "/basicPS.cso" };
		FILE* psFile{};

		strcpy(psShaderPath.data, shadersRegistryPath.data);
		strncat(psShaderPath.data, psRegistryPath, _countof(psRegistryPath));
		psShaderPath.num += _countof(psRegistryPath);

		int result{ fopen_s(&psFile, psShaderPath.data, "rb") };

		fseek(psFile, 0, SEEK_END);
		psByteSize = ftell(psFile);
		rewind(psFile);

		fread(psBlob, psByteSize, 1, psFile);
		fclose(psFile);
	}

	ComPtr<ID3DBlob> serializedBlob;
	ComPtr<ID3DBlob> errorBlob;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDsc{
		.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = {.NumParameters = 0,
				.pParameters = nullptr,
				.NumStaticSamplers = 0,
				.pStaticSamplers = 0,
				.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT}
	};

	D3D12SerializeVersionedRootSignature(&rsDsc, serializedBlob.GetAddressOf(), errorBlob.GetAddressOf());

	device->CreateRootSignature(0, serializedBlob->GetBufferPointer(), serializedBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	
	{
		
		const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = D3D12_SHADER_BYTECODE{ .pShaderBytecode = vsBlob, .BytecodeLength = vsByteSize };
		psoDesc.PS = D3D12_SHADER_BYTECODE{ .pShaderBytecode = psBlob, .BytecodeLength = psByteSize };
		psoDesc.BlendState = D3D12_BLEND_DESC{
				.AlphaToCoverageEnable = FALSE,
				.IndependentBlendEnable = FALSE };
		psoDesc.BlendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC{
			.BlendEnable = FALSE,
			.LogicOpEnable = FALSE,
			.SrcBlend = D3D12_BLEND_ONE,
			.DestBlend = D3D12_BLEND_ZERO,
			.BlendOp = D3D12_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D12_BLEND_ONE,
			.DestBlendAlpha = D3D12_BLEND_ZERO,
			.BlendOpAlpha = D3D12_BLEND_OP_ADD,
			.LogicOp = D3D12_LOGIC_OP_NOOP,
			.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
		};
		psoDesc.SampleMask = 0xffffffff;
		psoDesc.RasterizerState = D3D12_RASTERIZER_DESC{
		.FillMode = D3D12_FILL_MODE_SOLID,
		.CullMode = D3D12_CULL_MODE_BACK,
		.FrontCounterClockwise = FALSE,
		.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
		.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		.DepthClipEnable = TRUE,
		.MultisampleEnable = FALSE,
		.AntialiasedLineEnable = FALSE,
		.ForcedSampleCount = 0
		};
		psoDesc.DepthStencilState = D3D12_DEPTH_STENCIL_DESC{
			.DepthEnable = FALSE,
			.StencilEnable = FALSE
		};
		psoDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
			.pInputElementDescs = inputElementDescs, 
			.NumElements = _countof(inputElementDescs)
		};
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 };
		psoDesc.NodeMask = 0;
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;



		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

	}


	
	struct alignas(16) vtx {
		float3 pos;
		float3 color;
	};

	//Create staging buffer
	//Create resident buffer
	D3D12_RESOURCE_DESC stagingResDsc{
	.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
	.Alignment = 0,
	.Width = sizeof(vtx) * 3,
	.Height = 1,
	.DepthOrArraySize = 1,
	.MipLevels = 1,
	.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
	.SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
	.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	.Flags = D3D12_RESOURCE_FLAG_NONE };

	D3D12MA::ALLOCATION_DESC stagingAllocDsc{
		.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY,
		.HeapType = D3D12_HEAP_TYPE_UPLOAD,
		.ExtraHeapFlags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		.CustomPool = nullptr,
		.pPrivateData = nullptr
	};

	d3dmallocator->CreateResource(&stagingAllocDsc, &stagingResDsc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &vtxStagingAllocation, IID_PPV_ARGS(&vtxStagingBuffer));
	vtxStagingBuffer->SetName(L"StagingBuffer");

	D3D12MA::ALLOCATION_DESC residentAllocDsc{
		.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY,
		.HeapType = D3D12_HEAP_TYPE_DEFAULT,
		.ExtraHeapFlags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		.CustomPool = nullptr,
		.pPrivateData = nullptr
	};

	d3dmallocator->CreateResource(&residentAllocDsc, &stagingResDsc, D3D12_RESOURCE_STATE_COMMON, nullptr, &vtxResidentAllocation, IID_PPV_ARGS(&vtxResidentBuffer));
	vtxResidentBuffer->SetName(L"VtxResidentBuffer");
	
	{//copy to staging
		vtx  tri[3]
		{
			vtx{.pos = float3{0.f, 0.25f * ASPECT_RATIO, 0.f}, .color = float3{1.0f, 0.0f, 0.0f}},
			vtx{.pos = float3{0.25f, -0.25f * ASPECT_RATIO, 0.f}, .color = float3{0.0f, 1.0f, 0.0f}},
			vtx{.pos = float3{-0.25f, -0.25f * ASPECT_RATIO, 0.f}, .color = float3{0.0f, 0.0f, 1.0f}}
		};

		D3D12_RANGE mapRange{
			.Begin = 0,
			.End = sizeof(vtx) * 3
		};

		uint8* map;
		vtxStagingBuffer->Map(0, &mapRange, (void**)&map);
		memcpy(map, tri, sizeof(vtx) * 3);
		vtxStagingBuffer->Unmap(0, nullptr);

		//const D3D12_RESOURCE_BARRIER barrierDsc{
		//	.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		//	.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
		//	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
		//					.pResource = vtxStagingBuffer.Get(),
		//					.Subresource = 0,
		//					.StateBefore = D3D12_RESOURCE_STATE_COMMON,
		//					.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE}
		//};
		//
		////transition to copy src
		//directList->ResourceBarrier(1, &barrierDsc);
	}

	{//enqueue copy from staging to resident
		directList->CopyBufferRegion(vtxResidentBuffer.Get(), 0, vtxStagingBuffer.Get(), 0, sizeof(vtx) * 3);

		const D3D12_RESOURCE_BARRIER barrierDsc{
		.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
						.pResource = vtxResidentBuffer.Get(),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
						.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER}
		};
		directList->ResourceBarrier(1, &barrierDsc);

		//Create vtx buffer view
		vtxBufferView.BufferLocation = vtxResidentBuffer->GetGPUVirtualAddress();
		vtxBufferView.SizeInBytes = sizeof(vtx) * 3;
		vtxBufferView.StrideInBytes = sizeof(vtx);
	}
	

}


internal void Render(fp64 dt)
{
	WaitForFence(directFence.Get(), frameDirectFenceValue[currentFrame]);

	ID3D12CommandAllocator* frameAlloc{ commandAllocators[currentFrame].Get() };
	ID3D12GraphicsCommandList* frameList{ directList.Get() };
	ID3D12Resource* currentBackBuffer{ backBuffers[currentFrame].Get() };

	frameAlloc->Reset();
	frameList->Reset(frameAlloc, nullptr);

	//Clear the backbuffer

	{
		const D3D12_RESOURCE_BARRIER barrierDsc{
			.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
							.pResource = currentBackBuffer,
							.Subresource = 0,
							.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
							.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET}
		};
		frameList->ResourceBarrier(1, &barrierDsc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle{ rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	descriptorHandle.ptr += rtvDescriptorSize * currentFrame;




	{//Setup viewport
		D3D12_VIEWPORT viewport{
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width = static_cast<FLOAT>(windowRect.right - windowRect.left),
			.Height = static_cast<FLOAT>(windowRect.bottom - windowRect.top),
			.MinDepth = 0.f,
			.MaxDepth = 1.f
		};
		frameList->RSSetViewports(1, &viewport);
	}
	
	//Setup viewport scissor
	frameList->RSSetScissorRects(1, &windowRect);

	//Setup OM
	frameList->OMSetRenderTargets(1, &descriptorHandle, FALSE, nullptr);

	frameList->SetPipelineState(pso.Get());
	frameList->SetGraphicsRootSignature(rootSignature.Get());
	frameList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	frameList->IASetVertexBuffers(0, 1, &vtxBufferView);

	static constexpr float4 clearColor{ 0.0f, 0.2f, 0.4f, 1.0f };
	frameList->ClearRenderTargetView(descriptorHandle, &clearColor.x, 0, nullptr);
	frameList->DrawInstanced(3, 1, 0, 0);

		
	{
		const D3D12_RESOURCE_BARRIER barrierDsc{
			.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
							.pResource = currentBackBuffer,
							.Subresource = 0,
							.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
							.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT}
		};
		frameList->ResourceBarrier(1, &barrierDsc);
	}

	frameList->Close();

	ID3D12CommandList* commandLists[]{ frameList };
	directQueue->ExecuteCommandLists(1, commandLists);

	swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);


	const uint64 signalValue = frameDirectFenceValue[currentFrame] + 1;
	directQueue->Signal(directFence.Get(), signalValue);

	currentFrame = swapChain->GetCurrentBackBufferIndex();
	frameDirectFenceValue[currentFrame] = signalValue;
}


internal void UpdateApp(fp64 dt)
{
	Update(dt);
	Render(dt);
}

//process messages from kernel
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {



	switch (uMsg)
	{
	case WM_SIZE:
	{
		RECT clientRect = {};
		::GetClientRect(hwnd, &clientRect);

		int width = static_cast<int>(std::max(1L, clientRect.right - clientRect.left));
		int height = static_cast<int>(std::max(1L, clientRect.bottom - clientRect.top));

		DXGI_SWAP_CHAIN_DESC1 dsc;
		swapChain->GetDesc1(&dsc);

		if (dsc.Width != width || dsc.Height != height)
		{
			std::cout << "Resize { " << width << ", " << height << " }\n";
			//flush

			Flush(directQueue.Get(), directFence.Get(), frameDirectFenceValue[currentFrame]);

			//Reset all fences to the same value (basically scratch all frames and start over)
				for (int32 i{}; i < NUM_FRAMES; ++i)
				{
				frameDirectFenceValue[i] = frameDirectFenceValue[currentFrame];
					backBuffers[i].Reset();
				}

			swapChain->ResizeBuffers(NUM_FRAMES, width, height, dsc.Format, dsc.Flags);

			currentFrame = swapChain->GetCurrentBackBufferIndex();

			//recreate the RTV

			D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle{ rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
			for (int32 i{}; i < NUM_FRAMES; ++i)
			{
				swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
				device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, descriptorHandle);
				descriptorHandle.ptr += rtvDescriptorSize;
			}

			windowRect = clientRect;
		}

		
		break;
	}
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN:
	{
		if (wParam == 0x46 && !(lParam >> 30)) //key F is pressed and was up before
		{
			isFullscreen = isFullscreen ^ 1; //toggle it

			if (isFullscreen)
			{
				::GetWindowRect(hwnd, &windowRect);
				//idk XD -> we already have the old state of the rect so we can restore later
				UINT style{ WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX) };

				::SetWindowLong(hwnd, GWL_STYLE, style);
				HMONITOR hMonitor{ ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST) };
				MONITORINFOEX monitorInfo{};
				monitorInfo.cbSize = sizeof(MONITORINFOEX);
				::GetMonitorInfo(hMonitor, &monitorInfo);
				::SetWindowPos(hwnd, HWND_TOP,
					monitorInfo.rcMonitor.left,
					monitorInfo.rcMonitor.top,
					monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
					monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, 
					SWP_FRAMECHANGED | SWP_NOACTIVATE);
				::ShowWindow(hwnd, SW_MAXIMIZE);
				
			}
			else {
				//restore old state
				::SetWindowLongW(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				::SetWindowPos(hwnd, HWND_TOP,
					windowRect.left,
					windowRect.top,
					windowRect.right - windowRect.left,
					windowRect.bottom - windowRect.top,
					SWP_FRAMECHANGED | SWP_NOACTIVATE);
				::ShowWindow(hwnd, SW_NORMAL);

			}
		}
	}break;
	default:
	{
		return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	}


	return 0;
}



int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	//HINSTANCE instance{ ::GetModuleHandle(NULL) };

	AllocConsole();

	// Redirect STDOUT to the console
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);

	// Set std::cout and std::cin to use the console
	std::ios::sync_with_stdio();

	// Optional: set console title
	SetConsoleTitleW(L"Debug Console");

	{
		WNDCLASSEXW windowClass = {};
		const wchar_t* windowClassName = L"DX12WindowClass";
		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = &WndProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = hInstance;
		windowClass.hIcon = ::LoadIcon(hInstance, NULL);
		windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = windowClassName;
		windowClass.hIconSm = ::LoadIcon(hInstance, NULL);

		::SetProcessDPIAware();
		::RegisterClassExW(&windowClass);


		int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);


		//center it, please
		int32 xPos{ std::max<int32>(1, (screenWidth - INITIAL_WIDTH) / 2) };
		int32 yPos{ std::max<int32>(1, (screenHeight - INITIAL_HEIGHT) / 2) };

		RECT myRect{ 0, 0, INITIAL_WIDTH, INITIAL_HEIGHT };

		AdjustWindowRect(&myRect, WS_OVERLAPPEDWINDOW, FALSE);

		HWND hWnd = ::CreateWindowExW(
			NULL,
			windowClassName,
			WINDOW_NAME,
			WS_OVERLAPPEDWINDOW,
			xPos,
			yPos,
			myRect.right - myRect.left,
			myRect.bottom - myRect.top,
			NULL,
			NULL,
			hInstance,
			nullptr
		);

		::windowRect = myRect;


		ComPtr<ID3D12Debug> dbgLayer;

		D3D12GetDebugInterface(IID_PPV_ARGS(&dbgLayer));
		
		dbgLayer->EnableDebugLayer();
		
		{
			ComPtr<ID3D12Debug1> dbLayer1;
		
			dbgLayer.As(&dbLayer1);
		
		if (dbLayer1)
		{
			dbLayer1->SetEnableGPUBasedValidation(TRUE);
			dbLayer1->SetEnableSynchronizedCommandQueueValidation(TRUE);
		}
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgidebug)))) {
			}
		
		}
		{
			ComPtr<ID3D12Debug2> dbLayer2;
		
			dbgLayer.As(&dbLayer2);
		
			if (dbLayer2)
			{
				dbLayer2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
			}
		}
		{
			ComPtr<ID3D12Debug5> dbLayer5;
		
			dbgLayer.As(&dbLayer5);
		
			if (dbLayer5)
			{
				dbLayer5->SetEnableAutoName(TRUE);
			}
		}
		
		debugLayer = dbgLayer;

		//Create adapter, then create device by finding suitable adapter. Check for a factory and if we have factory6 query by preference
		ComPtr<IDXGIFactory4> dxgiFactory;


		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));

		ComPtr<IDXGIAdapter1> dxgiAdapter;
		ComPtr<ID3D12Device> d3Device;
		DXGI_ADAPTER_DESC1 adapterDesc;

		{
			//try to find factory6
			ComPtr<IDXGIFactory6> dxgiFactory6;
			dxgiFactory.As(&dxgiFactory6);


			HRESULT cond{};
			for (int32 i{}; ; ++i)
			{
				if (dxgiFactory6)
				{
					cond = dxgiFactory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter));
				}
				else
				{
					cond = dxgiFactory->EnumAdapters1(i, dxgiAdapter.ReleaseAndGetAddressOf());
				}

				if (cond == D3D12_ERROR_ADAPTER_NOT_FOUND)
					break;

				dxgiAdapter->GetDesc1(&adapterDesc);

				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}


				//try to create the device... if success... blabla
				if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_2, __uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&d3Device));

		bool supportsVRR{ false };

		{
			ComPtr<IDXGIFactory5> dxgiFactory5;
			dxgiFactory.As(&dxgiFactory5);
			if (dxgiFactory5)
			{
				BOOL vrr;
				dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE::DXGI_FEATURE_PRESENT_ALLOW_TEARING, &vrr, sizeof(vrr));

				supportsVRR = (vrr == TRUE);
			}
		}

		//Create the swap chain
		DXGI_SWAP_CHAIN_DESC1 swapchainDsc{
			.Width = INITIAL_WIDTH,
			.Height = INITIAL_HEIGHT,
			.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
			.Stereo = FALSE,
			.SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0},
			.BufferCount = NUM_FRAMES,
			.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_IGNORE,
			.Flags = supportsVRR ? DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u

		};
		//Command Queue
		ComPtr<ID3D12CommandQueue> d3dCommandQueue;
		D3D12_COMMAND_QUEUE_DESC commandQueueDsc{
			.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0 };
		d3Device->CreateCommandQueue(&commandQueueDsc, IID_PPV_ARGS(&d3dCommandQueue));

		ComPtr<IDXGISwapChain1> dxgiSwapChain;
		dxgiFactory->CreateSwapChainForHwnd(d3dCommandQueue.Get(), hWnd, &swapchainDsc, nullptr, nullptr, dxgiSwapChain.GetAddressOf());
		dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);


		ComPtr<IDXGISwapChain3> dxgiSwapChain3;
		dxgiSwapChain.As(&dxgiSwapChain3);




		//Command Allocator -> one foreach backbuffer

		uint8 acurrentFrame{ static_cast<uint8>(dxgiSwapChain3->GetCurrentBackBufferIndex()) };//this is actually not needed and we can just index-wrap


		ComPtr<ID3D12CommandAllocator> d3dAllocator[NUM_FRAMES];

		for (int32 i{}; i < NUM_FRAMES; ++i)
		{
			d3Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3dAllocator[i]));
			d3dAllocator[i]->Reset();
		}
		

		//Command List
		ComPtr<ID3D12GraphicsCommandList> d3dCommandList;

		d3Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dAllocator[currentFrame].Get(), nullptr, IID_PPV_ARGS(&d3dCommandList));


		ComPtr<ID3D12Resource> d3dRes[NUM_FRAMES];
		//Descriptor heap -> one foreach back buffer
		//Resource -> one foreach backbuffer -> they are created by the swapchain
		//we have to create the descriptor heap for the RTV, and then get the CPU descriptor handle to bind each
		//RTV of each buffer to a slot of the descriptor heap
		//Query the descriptor heap offset to know how much to move the pointers...

		ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorDsc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = NUM_FRAMES,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		d3Device->CreateDescriptorHeap(&descriptorDsc, IID_PPV_ARGS(&d3dDescriptorHeap));

		uint32 cpuHandleSize{ d3Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

		for (int32 i{}; i < NUM_FRAMES; ++i)
		{
			dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&d3dRes[i]));
			d3Device->CreateRenderTargetView(d3dRes[i].Get(), nullptr, cpuHandle);
			cpuHandle.ptr += cpuHandleSize;
		}
		//Fence -> one foreach queue

		ComPtr<ID3D12Fence> d3dFence;

		static constexpr int64 INITIAL_FENCE{ 0 };
		d3Device->CreateFence(INITIAL_FENCE, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFence));


		debugLayer = dbgLayer;
		factory = dxgiFactory;
		adapter = dxgiAdapter;
		device = d3Device;
		swapChain = dxgiSwapChain3;
		directFence = d3dFence;
		directList = d3dCommandList;
		directQueue = d3dCommandQueue;
		currentFrame = acurrentFrame;
		rtvDescriptorSize = cpuHandleSize;
		rtvDescriptorHeap = d3dDescriptorHeap;
		directFenceEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);;


		for (int32 i{}; i < NUM_FRAMES; ++i) {
			commandAllocators[i] = d3dAllocator[i];
			backBuffers[i] = d3dRes[i];
			frameDirectFenceValue[i] = INITIAL_FENCE;
		}
		D3D12MA::ALLOCATOR_DESC mallocatorDsc{
			.Flags = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS | D3D12MA::ALLOCATOR_FLAGS::ALLOCATOR_FLAG_SINGLETHREADED,
			.pDevice = device.Get(),
			.PreferredBlockSize = 0,
			.pAllocationCallbacks = nullptr,
			.pAdapter = adapter.Get()
		};

		D3D12MA::CreateAllocator(&mallocatorDsc, &d3dmallocator);


		




		::ShowWindow(hWnd, SW_SHOW);
	}

	dxgidebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);

	PrepInitialDataUpload();

	directList->Close();
	
	ID3D12CommandList* commandLists[]{ directList.Get()};
	
	directQueue->ExecuteCommandLists(1, commandLists);
	
	directQueue->Signal(directFence.Get(), ++frameDirectFenceValue[currentFrame]);
	
	
	
	std::chrono::system_clock::time_point prevFrame{ std::chrono::system_clock::now()};

	static constexpr fp64 targetMs{ 1000. / 60. }; //120fps


	MSG msg = { };
	bool running{ true };
	while (running)
	{
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}

			
		}
		std::chrono::system_clock::time_point currentFrame{ std::chrono::system_clock::now() };
		std::chrono::duration<fp64, std::milli> diff{ currentFrame - prevFrame };


		fp64 delta{ diff.count() };
		while (delta < targetMs)
		{
			std::this_thread::yield();
			currentFrame = std::chrono::system_clock::now();
			diff = currentFrame - prevFrame;
			delta = diff.count();
		}

		prevFrame = currentFrame;

		//std::cout << come mierdas << "\n";

		UpdateApp(delta);
	}
	


	Flush(directQueue.Get(), directFence.Get(), frameDirectFenceValue[currentFrame]);
	::CloseHandle(directFenceEvent);


	return 0;
}