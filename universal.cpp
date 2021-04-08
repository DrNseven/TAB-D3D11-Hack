//TAB D3D Multihack 0.8
//compile in release x64 mode
//chams does not work, use esp instead
//aimbot aims behind walls use small aimfov

#pragma once
#include <Windows.h>
#include <vector>
#include <mutex>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcommon.h>
#include <D3Dcompiler.h> //generateshader
#pragma comment(lib, "D3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib") //timeGetTime
#include "MinHook/include/MinHook.h" //detour x86&x64

//imgui
#include "ImGui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "ImGui\imgui_impl_dx11.h"

//DX Includes
#include <DirectXMath.h>
using namespace DirectX;

#pragma warning( disable : 4244 )


typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall *D3D11ResizeBuffersHook) (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

typedef void(__stdcall *D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);


D3D11PresentHook phookD3D11Present = NULL;
D3D11ResizeBuffersHook phookD3D11ResizeBuffers = NULL;

D3D11DrawIndexedHook phookD3D11DrawIndexed = NULL;


ID3D11Device *pDevice = NULL;
ID3D11DeviceContext *pContext = NULL;

DWORD_PTR* pSwapChainVtable = NULL;
DWORD_PTR* pContextVTable = NULL;
DWORD_PTR* pDeviceVTable = NULL;


#include "main.h" //helper funcs

//==========================================================================================================================

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND window = NULL;
WNDPROC oWndProc;

void InitImGuiD3D11()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return true;
	}
	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

//==========================================================================================================================

HRESULT __stdcall hookD3D11ResizeBuffers(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	ImGui_ImplDX11_InvalidateDeviceObjects();
	if (nullptr != mainRenderTargetViewD3D11) { mainRenderTargetViewD3D11->Release(); mainRenderTargetViewD3D11 = nullptr; }

	HRESULT toReturn = phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ImGui_ImplDX11_CreateDeviceObjects();

	return phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

//==========================================================================================================================

HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!initonce)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGuiD3D11();

			// Create depthstencil state
			D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
			depthStencilDesc.DepthEnable = TRUE;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			depthStencilDesc.StencilEnable = FALSE;
			depthStencilDesc.StencilReadMask = 0xFF;
			depthStencilDesc.StencilWriteMask = 0xFF;
			// Stencil operations if pixel is front-facing
			depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			// Stencil operations if pixel is back-facing
			depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
			depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			pDevice->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState_FALSE);

			//Create depth stencil state
			D3D11_DEPTH_STENCIL_DESC depthstencildesc2;
			ZeroMemory(&depthstencildesc2, sizeof(D3D11_DEPTH_STENCIL_DESC));
			depthstencildesc2.DepthEnable = true;
			depthstencildesc2.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
			depthstencildesc2.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER; //D3D11_COMPARISON_LESS_EQUAL;
			pDevice->CreateDepthStencilState(&depthstencildesc2, &DepthStencilState_TRUE);

			//GenerateTexture(0xff00ff00, DXGI_FORMAT_R10G10B10A2_UNORM); //DXGI_FORMAT_R32G32B32A32_FLOAT); //DXGI_FORMAT_R8G8B8A8_UNORM; 

			//load cfg settings
			LoadCfg();

			initonce = true;
		}
		else
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);
	}

	//create shaders
	//if (!sGreen)
		//GenerateShader(pDevice, &sGreen, 0.0f, 1.0f, 0.0f); //green

	//if (!sMagenta)
		//GenerateShader(pDevice, &sMagenta, 1.0f, 0.0f, 1.0f); //magenta

	//recreate rendertarget on reset
	if (mainRenderTargetViewD3D11 == NULL)
	{
		ID3D11Texture2D* pBackBuffer = NULL;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
		pBackBuffer->Release();
	}

	//get imgui displaysize
	ImGuiIO io = ImGui::GetIO();
	ViewportWidth = io.DisplaySize.x;
	ViewportHeight = io.DisplaySize.y;
	ScreenCenterX = ViewportWidth / 2.0f;
	ScreenCenterY = ViewportHeight / 2.0f;

	if (GetAsyncKeyState(VK_INSERT) & 1) {
		SaveCfg(); //save settings
		showmenu = !showmenu;
	}


	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (showmenu) {

		ImGui::Begin("Hack Menu");
		ImGui::Checkbox("Wallhack", &wallhack);
		//ImGui::Checkbox("Chams", &chams);
		ImGui::Checkbox("Esp", &esp);
		ImGui::SliderInt("Espsize", &espsize, 100, 1000);
		//circle esp
		//line esp
		//text esp
		//distance esp
		ImGui::Checkbox("Crosshair", &crosshair);
		//crosshair size 32
		ImGui::Checkbox("Aimbot", &aimbot);
		ImGui::SliderFloat("Aimsens", &aimsens, 0, 20);
		ImGui::Text("Aimkey");
		const char* aimkey_Options[] = { "Shift", "Right Mouse", "Left Mouse", "Middle Mouse", "Ctrl", "Alt", "Capslock", "Space", "X", "C", "V" };
		ImGui::SameLine();
		ImGui::Combo("##AimKey", (int*)&aimkey, aimkey_Options, IM_ARRAYSIZE(aimkey_Options));
		ImGui::SliderInt("Aimfov", &aimfov, 0, 10);
		ImGui::SliderInt("Aimspeed uses distance", &aimspeed_uses_dst, 0, 4);
		ImGui::SliderInt("Aimspeed", &aimspeed, 0, 100);
		ImGui::SliderFloat("Aimheight", &aimheight, -5, 5);
		ImGui::SliderFloat("Preaim", &preaim, 0, 5);
		ImGui::SliderInt("Autoshoot", &autoshoot, 0, 2);//autoshoot 1 = always stop shooting, autoshoot 2 = only stop if key released or no target (1 is good for valorant, 2 for quake)
		ImGui::SliderInt("As xhair dst", &as_xhairdst, 0, 20);
		//as_compensatedst
		ImGui::SliderInt("As delay", &asdelay, 0, 200);
		ImGui::SliderInt("testvalue", &testvalue, 100, 200);

		ImGui::End();
	}


	targetfound = false;
	//do esp
	if (esp == 1)
	{
		ImGui::Begin("Transparent", reinterpret_cast<bool*>(true), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
		ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

		if (AimEspInfo.size() != NULL)
		{
			for (unsigned int i = 0; i < AimEspInfo.size(); i++)
			{
				if (AimEspInfo[i].vOutX > 1 && AimEspInfo[i].vOutY > 1 && AimEspInfo[i].vOutX < ViewportWidth && AimEspInfo[i].vOutY < ViewportHeight)
				{
					//text esp
					//ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(AimEspInfo[i].vOutX, AimEspInfo[i].vOutY), ImColor(255, 255, 255, 255), "[]", 0, 0.0f, 0); //draw text

					//draw cricle
					ImGui::GetWindowDrawList()->AddCircle(ImVec2(AimEspInfo[i].vOutX, AimEspInfo[i].vOutY), espsize/AimEspInfo[i].vOutZ, IM_COL32(255, 255, 255, 255), 12, 2.0f); //scale with distance
				}
			}
		}
		ImGui::End();
	}

	if (aimkey == 0) Daimkey = VK_SHIFT;
	if (aimkey == 1) Daimkey = VK_RBUTTON;
	if (aimkey == 2) Daimkey = VK_LBUTTON;
	if (aimkey == 3) Daimkey = VK_MBUTTON;
	if (aimkey == 4) Daimkey = VK_CONTROL;
	if (aimkey == 5) Daimkey = VK_MENU;
	if (aimkey == 6) Daimkey = VK_CAPITAL;
	if (aimkey == 7) Daimkey = VK_SPACE;
	if (aimkey == 8) Daimkey = 0x58; //X
	if (aimkey == 9) Daimkey = 0x43; //C
	if (aimkey == 10) Daimkey = 0x56; //V

	//aimbot
	if (aimbot == 1)//aimbot pve, aimbot pvp
		if (AimEspInfo.size() != NULL)
		{
			UINT BestTarget = -1;
			DOUBLE fClosestPos = 99999;

			for (unsigned int i = 0; i < AimEspInfo.size(); i++)
			{
				//aimfov
				float radiusx = (aimfov * 10.0f) * (ScreenCenterX / 100.0f);
				float radiusy = (aimfov * 10.0f) * (ScreenCenterY / 100.0f);

				//get crosshairdistance
				AimEspInfo[i].CrosshairDst = GetDst(AimEspInfo[i].vOutX, AimEspInfo[i].vOutY, ViewportWidth / 2.0f, ViewportHeight / 2.0f);

				//if in fov
				if (AimEspInfo[i].vOutX >= ScreenCenterX - radiusx && AimEspInfo[i].vOutX <= ScreenCenterX + radiusx && AimEspInfo[i].vOutY >= ScreenCenterY - radiusy && AimEspInfo[i].vOutY <= ScreenCenterY + radiusy)

					//get closest/nearest target to crosshair
					if (AimEspInfo[i].CrosshairDst < fClosestPos)
					{
						fClosestPos = AimEspInfo[i].CrosshairDst;
						BestTarget = i;
					}
			}

			//if nearest target to crosshair
			if (BestTarget != -1)
			{
				//aim
				if (GetAsyncKeyState(Daimkey) & 0x8000)
					AimAtPos(AimEspInfo[BestTarget].vOutX, AimEspInfo[BestTarget].vOutY);

				//get crosshairdistance
				//AimEspInfo[BestTarget].CrosshairDst = GetDst(AimEspInfo[BestTarget].vOutX, AimEspInfo[BestTarget].vOutY, ViewportWidth / 2.0f, ViewportHeight / 2.0f);

				//stabilise aim
				if (aimspeed_uses_dst == 0) //default steady aimsens
					AimSpeed = aimsens;
				else if (aimspeed_uses_dst == 1)
					AimSpeed = aimsens + (AimEspInfo[BestTarget].CrosshairDst * 0.008f); //0.01f the bigger the distance the slower the aimspeed
				else if (aimspeed_uses_dst == 2)
					AimSpeed = aimsens + (AimEspInfo[BestTarget].CrosshairDst * 0.01f); //0.01f the bigger the distance the slower the aimspeed
				else if (aimspeed_uses_dst == 3)
					AimSpeed = aimsens + (AimEspInfo[BestTarget].CrosshairDst * 0.012f); //0.01f the bigger the distance the slower the aimspeed
					//AimSpeed = aimsens + (rand() % 100 / CrosshairDst);     
				else if (aimspeed_uses_dst == 4)
				{
					AimSpeed = aimsens + (75.0f / AimEspInfo[BestTarget].CrosshairDst); //100.0f the bigger the distance the faster the aimspeed
					//AimSpeed = aimsens + (50.0f / CrosshairDst); //50.0f the bigger the distance the faster the aimspeed
					//float randomnb = rand() % 2; //both
					//if (randomnb == 0) AimSpeed = aimsens + (75.0f / CrosshairDst); //the bigger the distance the faster the aimspeed
					//else if (randomnb == 1) AimSpeed = aimsens + (CrosshairDst * 0.01f); //the bigger the distance the slower the aimspeed
				}

				//autoshoot on
				if (autoshoot > 0 && !IsPressed && !GetAsyncKeyState(VK_LBUTTON) && GetAsyncKeyState(Daimkey) & 0x8000 && AimEspInfo[BestTarget].CrosshairDst <= as_xhairdst)//if crosshairdst smaller than as_xhairdist then fire                            
				{
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					IsPressed = true;
				}
			}
		}
	AimEspInfo.clear();

	//autoshoot off
	if ((autoshoot == 2 && IsPressed && !targetfound || autoshoot == 2 && IsPressed) || //always stop shooting
		(autoshoot == 1 && IsPressed && !targetfound || autoshoot == 1 && IsPressed && !GetAsyncKeyState(aimkey)))
	{
		if (timeGetTime() - dwLastAction >= (unsigned int)asdelay) //wait 
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			IsPressed = false;

			dwLastAction = timeGetTime();
		}
	}

	if (crosshair)
		DrawCrosshair();

	//ImGui::EndFrame();
	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetViewD3D11, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}
//==========================================================================================================================

void __stdcall hookD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	ID3D11Buffer* veBuffer;
	UINT veWidth;
	UINT Stride;
	UINT veBufferOffset;
	D3D11_BUFFER_DESC veDesc;

	//get buffers
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
	if (veBuffer) {
		veBuffer->GetDesc(&veDesc);
		veWidth = veDesc.ByteWidth;
	}
	if (NULL != veBuffer) {
		veBuffer->Release();
		veBuffer = NULL;
	}

	if (Stride == 40 && IndexCount == 2820)
	pContext->PSGetShaderResources(0, 1, &psShaderResourceView0);
	if (psShaderResourceView0)
	{
		psShaderResourceView0->GetDesc(&psDescr0);
	}
	SAFE_RELEASE(psShaderResourceView0);

	if (Stride == 40 && IndexCount == 2820)
	pContext->PSGetShaderResources(1, 1, &psShaderResourceView1);
	if (psShaderResourceView1)
	{
		psShaderResourceView1->GetDesc(&psDescr1);
	}
	SAFE_RELEASE(psShaderResourceView1);


	//wallhack/chams
	if (wallhack == 1 || chams == 1) //if wallhack or chams option is enabled in menu
	if (Stride == 40 && IndexCount == 2820 && veWidth == 70920 && psDescr0.Format == 29 && psDescr1.Format == 29)//tab models
		{
			//get orig
			if (wallhack == 1)
				pContext->OMGetDepthStencilState(&DepthStencilState_ORIG, &stencilRef); //get original

			//set off
			if (wallhack == 1)
				pContext->OMSetDepthStencilState(DepthStencilState_FALSE, 192); //depthstencil off

			//if (chams == 1)
			//{
				//pContext->PSSetShader(sGreen, NULL, NULL);
				//pContext->PSSetShaderResources(0, 1, &textureColor); 
			//}

			phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw

			//if (chams == 1)
				//pContext->PSSetShader(sMagenta, NULL, NULL);

			//restore orig
			if (wallhack == 1)
				pContext->OMSetDepthStencilState(DepthStencilState_ORIG, 192); //depthstencil on

			//if (wallhack == 1)
				//pContext->OMSetDepthStencilState(DepthStencilState_TRUE, 0); //depthstencil on alternative

			//release
			if (wallhack == 1)
				SAFE_RELEASE(DepthStencilState_ORIG); //release
		}

	//esp/aimbot
	if (esp == 1 || aimbot == 1) //if esp/aimbot option is enabled in menu
	if (Stride == 40 && IndexCount == 2820 && veWidth == 70920 && psDescr0.Format == 29 && psDescr1.Format == 29)//paste Format 29 29 to help performace
		{
			AddModel(pContext); //w2s
			targetfound = true;
		}


	//log stencilRef to fix blending bug with wallhack, log Format to help performance
	//if (Stride == 40 && IndexCount == 2820 && veWidth == 70920)//tab models 95 fps at test spot
		//if (GetAsyncKeyState(VK_F10) & 1) //press F10 to log to log.txt
			//Log("Stride == %d && IndexCount == %d && veWidth == %d && psDescr0.Format == %d && psDescr1.Format == %d && vsDescr0.Format == %d && vsDescr1.Format == %d && stencilRef == %d", 
			//Stride, IndexCount, veWidth, psDescr0.Format, psDescr1.Format, vsDescr0.Format, vsDescr1.Format, stencilRef);


    return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

//==========================================================================================================================

const int MultisampleCount = 1; // Set to 1 to disable multisampling
LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){ return DefWindowProc(hwnd, uMsg, wParam, lParam); }
DWORD __stdcall InitializeHook(LPVOID)
{
	HMODULE hDXGIDLL = 0;
	do
	{
		hDXGIDLL = GetModuleHandle("dxgi.dll");
		Sleep(4000);
	} while (!hDXGIDLL);
	Sleep(100);

	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

    IDXGISwapChain* pSwapChain;

	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

	D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
	D3D_FEATURE_LEVEL obtainedLevel;
	ID3D11Device* d3dDevice = nullptr;
	ID3D11DeviceContext* d3dContext = nullptr;

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = MultisampleCount;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

	scd.BufferDesc.Width = 1;
	scd.BufferDesc.Height = 1;
	scd.BufferDesc.RefreshRate.Numerator = 0;
	scd.BufferDesc.RefreshRate.Denominator = 1;

	UINT createFlags = 0;
#ifdef _DEBUG
	// This flag gives you some quite wonderful debug text. Not wonderful for performance, though!
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGISwapChain* d3dSwapChain = 0;

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		requestedLevels,
		sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION,
		&scd,
		&pSwapChain,
		&pDevice,
		&obtainedLevel,
		&pContext)))
	{
		MessageBox(hWnd, "Failed to create directX device and swapchain!", "Error", MB_ICONERROR);
		return NULL;
	}


    pSwapChainVtable = (DWORD_PTR*)pSwapChain;
    pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

    pContextVTable = (DWORD_PTR*)pContext;
    pContextVTable = (DWORD_PTR*)pContextVTable[0];

	pDeviceVTable = (DWORD_PTR*)pDevice;
	pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];

	if (MH_Initialize() != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[8], hookD3D11Present, reinterpret_cast<void**>(&phookD3D11Present)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[13], hookD3D11ResizeBuffers, reinterpret_cast<void**>(&phookD3D11ResizeBuffers)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[13]) != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pContextVTable[12], hookD3D11DrawIndexed, reinterpret_cast<void**>(&phookD3D11DrawIndexed)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return 1; }	
	
    DWORD dwOld;
    VirtualProtect(phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	while (true) {
		Sleep(10);
	}

	pDevice->Release();
	pContext->Release();
	pSwapChain->Release();

    return NULL;
}

//==========================================================================================================================

BOOL __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{ 
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH: // A process is loading the DLL.
		DisableThreadLibraryCalls(hModule);
		GetModuleFileName(hModule, dlldir, 512);
		for (size_t i = strlen(dlldir); i > 0; i--) { if (dlldir[i] == '\\') { dlldir[i + 1] = 0; break; } }
		CreateThread(NULL, 0, InitializeHook, NULL, 0, NULL);
		break;

	case DLL_PROCESS_DETACH: // A process unloads the DLL.
		if (MH_Uninitialize() != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pSwapChainVtable[13]) != MH_OK) { return 1; }

		if (MH_DisableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return 1; }

		break;
	}
	return TRUE;
}

// Exported function for SetWindowsHookEx injector
extern "C" __declspec(dllexport) int NextHook(int code, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(NULL, code, wParam, lParam);
}