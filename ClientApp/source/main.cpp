//  Filename: main.cpp
//	Author:	Daniel														
//	Date: 17/04/2025 21:00:06		
//  Sqwack-Studios													

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <utility>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <execution>
#include <shellapi.h> // For CommandLineToArgvW

using namespace Microsoft::WRL;

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#include "RadiantEngine/core/types.h"
#include "RadiantEngine/math/floatN.h"

using namespace RE;

static constexpr fp32 ASPECT_RATIO{ 16.f / 9.f};
static constexpr int16 WINDOW_HEIGHT{ 1080 };
static constexpr int16 WINDOW_WIDTH{ static_cast<int16>(WINDOW_HEIGHT * ASPECT_RATIO) };
static constexpr int8 NUM_FRAMES{ 3 };
static constexpr wchar_t WINDOW_NAME[]{ L"FedeGordo" };


ComPtr<IDXGIFactory4> dxgiFactory;
ComPtr<IDXGISwapChain4> dxgiSwapChain;

ComPtr<ID3D12Debug> d3dDebug;

ComPtr<ID3D12Device2> d3d12Device;

ComPtr<ID3D12Resource> d3dBackBuffers[NUM_FRAMES];
ComPtr<ID3D12Fence> fence;
uint64 currentFence;
uint64 frameFenceValues[NUM_FRAMES];
HANDLE fenceEvent;

uint8 currentFrame;
uint32 rtvDescriptorHeapSize;
ComPtr<ID3D12CommandQueue> d3dRenderCommandQueue;
ComPtr<ID3D12GraphicsCommandList> d3dRenderCommandList;
ComPtr<ID3D12CommandAllocator> d3dCommandAlloc[NUM_FRAMES];
ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap;

constexpr float4 red{ 1.0f, 0.0f, 0.0f, 1.0f };
constexpr float4 green{ 0.f, 1.0f, 0.0f, 1.0f };
constexpr float4 blue{ 0.f, 0.f, 1.0f, 1.0f };
constexpr float4 black{ 0.0f, 0.0f, 0.0f, 1.0f };

float4 clearColors[NUM_FRAMES]{ red, green, blue };

static void Update();// "game logic"
static void Render();// "render thread, create commants, handle resource creations, fences, present, etc

static void Flush(ID3D12CommandQueue*, ID3D12Fence*, uint64);
static void WaitForFence(ID3D12Fence*, uint64);


static void WaitForFence(ID3D12Fence* fence, uint64 valueToWaitFor)
{
	if (fence->GetCompletedValue() < valueToWaitFor)
	{
		fence->SetEventOnCompletion(valueToWaitFor, fenceEvent);
		::WaitForSingleObject(fenceEvent, 10000);
	}
}
static void Flush(ID3D12CommandQueue* queue, ID3D12Fence* fence, uint64 fenceValue)
{
	queue->Signal(fence, fenceValue);
	WaitForFence(fence, fenceValue);
}


static void Update()
{
	Sleep(1000);
	return;
}

static void Render()
{
	// Cleanup command allocator and list for this frame to start recording
	ComPtr<ID3D12CommandAllocator> commAlloc{ d3dCommandAlloc[currentFrame] };
	//Get a reference to this frame back buffer to cleanup the RTV
	ComPtr<ID3D12Resource> backBuffer{ d3dBackBuffers[currentFrame] };
	

	commAlloc->Reset();
	d3dRenderCommandList->Reset(commAlloc.Get(), NULL);

	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
				.pResource = backBuffer.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
				.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET
			}
		};
		//Transition resources from Present->RTV
		d3dRenderCommandList->ResourceBarrier(1, &barrier);

	}
	//clear backbuffer
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferDescriptorHnd{ d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };

	backBufferDescriptorHnd.ptr += currentFrame * rtvDescriptorHeapSize;

	d3dRenderCommandList->ClearRenderTargetView(backBufferDescriptorHnd, &clearColors[currentFrame].x, 0, nullptr);

	{
		D3D12_RESOURCE_BARRIER barrier{
			.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = D3D12_RESOURCE_TRANSITION_BARRIER{
				.pResource = backBuffer.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
				.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT
			}
		};

		d3dRenderCommandList->ResourceBarrier(1, &barrier);
	}
	//Submit the list to the queue
	d3dRenderCommandList->Close();

	ID3D12CommandList* const commandLists[]{
			d3dRenderCommandList.Get()
	};

	d3dRenderCommandQueue->ExecuteCommandLists(1, commandLists);


	//Signal the fence
	//We signal the fence value to +1 the current fence value
	uint64 fenceToWaitFor{ ++currentFence };
	frameFenceValues[currentFrame] = fenceToWaitFor;
	d3dRenderCommandQueue->Signal(fence.Get(), fenceToWaitFor);

	dxgiSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

	//refresh current back buffer index
	uint8 nextFrameIdx{ static_cast<uint8>(dxgiSwapChain->GetCurrentBackBufferIndex()) };
	currentFrame = nextFrameIdx;

	//Wait for fence
	WaitForFence(fence.Get(), fenceToWaitFor);
	






}
//process messages from kernel
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {



	switch (uMsg)
	{
	case WM_PAINT:
	{
		Update();
		Render();
		break;
	}
	case WM_SIZE:
	{
		//RECT clientRect = {};
		//::GetClientRect(g_hWnd, &clientRect);
		//
		//int width = clientRect.right - clientRect.left;
		//int height = clientRect.bottom - clientRect.top;

		//Resize(width, height);
		break;
	}
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	default:
	{
		return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	}

	return 0;
}






static HRESULT queryAdapter(const int32 i, IDXGIAdapter1** adapter, IDXGIFactory6* const factory6)
{
	HRESULT result{ 0 };
	if (factory6)
	{
		result = factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(adapter));
	}
	else
	{
		result = dxgiFactory->EnumAdapters1(i, adapter);
	}

	return result;
}


int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	//HINSTANCE instance{ ::GetModuleHandle(NULL) };

	WNDCLASSEXW windowClass = {};
	const wchar_t* windowClassName = L"DX12WindowClass";
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
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

	::RegisterClassExW(&windowClass);

	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(WINDOW_WIDTH), static_cast<LONG>(WINDOW_HEIGHT) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	int  windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowExW(
		NULL,
		windowClassName,
		WINDOW_NAME,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		hInstance,
		nullptr
	);

	//Initialize Debug layer
	UINT factoryFlags{ DXGI_CREATE_FACTORY_DEBUG };
	CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&dxgiFactory));

	D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug));
	d3dDebug->EnableDebugLayer();

	{
		ComPtr<ID3D12Debug1> dbg1;
		if (SUCCEEDED(d3dDebug.As(&dbg1)))
		{
			dbg1->SetEnableGPUBasedValidation(true);
			dbg1->SetEnableSynchronizedCommandQueueValidation(true);
		}
	}

	{
		ComPtr<ID3D12Debug2> dbg2;
		if (SUCCEEDED(d3dDebug.As(&dbg2)))
		{
			dbg2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
		}
	}

	{
		ComPtr<ID3D12Debug4> dbg4;
		if (SUCCEEDED(d3dDebug.As(&dbg4)))
		{
			//Not useful actually, only if we plan to "switch" the debug layer on the fly to improve performance on debug builds, but for that device removal must happen
			//dbg4->DisableDebugLayer();
		}
	}

	{
		ComPtr<ID3D12Debug5> dbg5;
		if (SUCCEEDED(d3dDebug.As(&dbg5)))
		{
			dbg5->SetEnableAutoName(true);
		}
	}



	

	ComPtr<IDXGIAdapter1> adapter1;
	DXGI_ADAPTER_DESC1 adapterDesc;
	{
		ComPtr<IDXGIFactory6> factory6{ nullptr };
		constexpr D3D_FEATURE_LEVEL desiredD3D12{ D3D_FEATURE_LEVEL_12_2 };

		dxgiFactory.As(&factory6);

		for (int32 i{}; queryAdapter(i, adapter1.ReleaseAndGetAddressOf(), factory6.Get()) != D3D12_ERROR_ADAPTER_NOT_FOUND; ++i)
		{
			adapter1->GetDesc1(&adapterDesc);

			//discard software adapters

			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;



			if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), desiredD3D12, __uuidof(ID3D12Device2), nullptr)))
			{
				break;
			}
		}

		HRESULT res{ D3D12CreateDevice(adapter1.Get(), desiredD3D12, IID_PPV_ARGS(&d3d12Device)) };
	}


	//Extract information of the GPU
	// ...
	// ...
	//Extract extra information, I have no idea what is this for 
	{
		ComPtr<IDXGIAdapter2> adapter2;
		if (SUCCEEDED(adapter1.As(&adapter2)))
		{
			DXGI_ADAPTER_DESC2 desc2;
			adapter2->GetDesc2(&desc2);

		}

		ComPtr<IDXGIAdapter4> adapter4;
		if (SUCCEEDED(adapter1.As(&adapter4)))
		{
			DXGI_ADAPTER_DESC3 desc3;
			adapter4->GetDesc3(&desc3);

		}
	}

	{
		ComPtr<ID3D12InfoQueue> infoQueue;

		if (SUCCEEDED(d3d12Device.As(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			constexpr uint8 MAX_SEVERITIES{ 5 };
			D3D12_MESSAGE_SEVERITY severities[MAX_SEVERITIES]{
				D3D12_MESSAGE_SEVERITY_CORRUPTION,
				D3D12_MESSAGE_SEVERITY_ERROR,
				D3D12_MESSAGE_SEVERITY_MESSAGE,
				D3D12_MESSAGE_SEVERITY_WARNING,
				D3D12_MESSAGE_SEVERITY_INFO
			};

			// Suppress individual messages by their ID
			D3D12_MESSAGE_ID deniedIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			};

			D3D12_INFO_QUEUE_FILTER_DESC infoDesc{
				.NumCategories = 0,
				.pCategoryList = nullptr,
				.NumSeverities = MAX_SEVERITIES,
				.pSeverityList = severities,
				.NumIDs = 0,
				.pIDList = deniedIds,
			};


		}
	}



	bool supportsVRR{ false }; //Variable Refresh Rate

	ComPtr<IDXGIFactory5> dxgiFactory5;
	if (SUCCEEDED(dxgiFactory.As(&dxgiFactory5)))
	{
		BOOL featureAvailable{ FALSE };
		SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &featureAvailable, sizeof(BOOL)));
		supportsVRR = featureAvailable == TRUE;

	}

	D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {
	.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
	.Support1 = D3D12_FORMAT_SUPPORT1_DISPLAY
	};

	HRESULT cogno = d3d12Device->CheckFeatureSupport(
		D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport));



	{
		D3D12_COMMAND_QUEUE_DESC commandQDsc{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask = 0 /* only needed if we use more GPUs, this defines the adapter to target */
		};

		d3d12Device->CreateCommandQueue(&commandQDsc, IID_PPV_ARGS(&d3dRenderCommandQueue));
	}

	{
		//Create SwapChain

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = WINDOW_WIDTH;
		swapChainDesc.Height = WINDOW_HEIGHT;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc = { 1, 0 };
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = NUM_FRAMES;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = supportsVRR ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> tmpSwapChain;


		HRESULT swapChainRes{ dxgiFactory->CreateSwapChainForHwnd(d3dRenderCommandQueue.Get(), hWnd, &swapChainDesc, NULL, NULL, tmpSwapChain.GetAddressOf()) };
		tmpSwapChain.As(&dxgiSwapChain);
		currentFrame = static_cast<uint8>(dxgiSwapChain->GetCurrentBackBufferIndex());

	}


	//Create the descriptor heap to hold swap chain's rtv
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = NUM_FRAMES,
		.NodeMask = 0
	};

	d3d12Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&d3dDescriptorHeap));

	rtvDescriptorHeapSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{ d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };


	//now we create each RTV to each back buffer of the swap chain.
	// With the handle, we get a pointer to each descriptor, so during RTV creation we pass the handle
	// so the descriptor of the RTV is properly offset
	for (int32 i{}; i < NUM_FRAMES; ++i)
	{
		ComPtr<ID3D12Resource> bufferRes;

		dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&bufferRes));

		d3d12Device->CreateRenderTargetView(bufferRes.Get(), NULL /*Null to use the descriptor of the resource*/, cpuDescriptorHandle);
		cpuDescriptorHandle.ptr += rtvDescriptorHeapSize; //ofset the handle for the next rtv creation
		d3dBackBuffers[i] = bufferRes;
	}


	for (int32 i{}; i < NUM_FRAMES; ++i)
	{
		d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3dCommandAlloc[i]));
	}


	d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dCommandAlloc[currentFrame].Get(), NULL, IID_PPV_ARGS(&d3dRenderCommandList));
	d3dRenderCommandList->Close();


	d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	::ShowWindow(hWnd, SW_SHOW);


	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Flush(d3dRenderCommandQueue.Get(), fence.Get(), ++currentFence);

	::CloseHandle(fenceEvent);

	return 0;
}