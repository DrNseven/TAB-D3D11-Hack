//main.h

#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;


//globals
bool showmenu = false;
bool initonce = false;

//item states
bool wallhack = 0;
bool chams = 0;
bool esp = 1;
int espsize = 500;
bool crosshair = 1;
bool aimbot = 1;
int aimfov = 3;
int aimkey = 0;
float aimsens = 0.8f;
int aimspeed_uses_dst = 1;		
float AimSpeed;                             
int aimspeed = 0;                           //5 slow dont move mouse faster than 5 pixel, 100 fast
float aimheight = 0.7f;						//aim height value
float preaim = 0.1f;						//aim a little in front of target to not aim behind
int autoshoot = 0;							//autoshoot
int as_xhairdst = 7.0;						//autoshoot activates below this crosshair distance
int asdelay = 0;							//wait ms before MOUSEEVENTF_LEFTUP
DWORD dwLastAction = timeGetTime();         //as timer
int testvalue=0;

DWORD Daimkey = VK_SHIFT;					//aimkey
bool IsPressed = false;
bool targetfound = false;

bool models = false;
UINT mStride;

//rendertarget
ID3D11RenderTargetView* mainRenderTargetViewD3D11;

//wh
UINT stencilRef = 0;
ID3D11DepthStencilState* DepthStencilState_TRUE = NULL; //depth off
ID3D11DepthStencilState* DepthStencilState_FALSE = NULL; //depth off
ID3D11DepthStencilState* DepthStencilState_ORIG = NULL; //depth on

//shader
ID3D11PixelShader* sGreen = NULL;
ID3D11PixelShader* sMagenta = NULL;

//pssetshaderresources
D3D11_SHADER_RESOURCE_VIEW_DESC psDescr0;
D3D11_SHADER_RESOURCE_VIEW_DESC psDescr1;
//D3D11_TEXTURE2D_DESC texdesc;
ID3D11ShaderResourceView* psShaderResourceView0;
ID3D11ShaderResourceView* psShaderResourceView1;

//Viewport
float ViewportWidth;
float ViewportHeight;
float ScreenCenterX;
float ScreenCenterY;

//==========================================================================================================================

//get dir
using namespace std;
#include <fstream>

// getdir & log
char dlldir[512];
char* GetDirFile(char* name)
{
	static char pldir[512];
	strcpy_s(pldir, dlldir);
	strcat_s(pldir, name);
	return pldir;
}

//log
void Log(const char* fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirFile((PCHAR)"log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}


#include <string>
void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirFile("w2sf.ini"), ios::trunc);
	fout << "Wallhack " << wallhack << endl;
	//fout << "Chams " << chams << endl;
	fout << "Esp " << esp << endl;
	fout << "Espsize " << espsize << endl;
	fout << "Aimbot " << aimbot << endl;
	fout << "Aimkey " << aimkey << endl;
	fout << "Aimfov " << aimfov << endl;
	fout << "Preaim " << preaim << endl;
	fout << "Aimspeed uses dst " << aimspeed_uses_dst << endl;
	fout << "Aimspeed " << aimspeed << endl;
	fout << "Aimheight " << aimheight << endl;
	fout << "Crosshair " << crosshair << endl;
	fout << "Autoshoot " << autoshoot << endl;
	fout << "AsXhairdst " << as_xhairdst << endl;
	fout << "As delay " << asdelay << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirFile("w2sf.ini"), ifstream::in);
	fin >> Word >> wallhack;
	//fin >> Word >> chams;
	fin >> Word >> esp;
	fin >> Word >> espsize;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimfov;
	fin >> Word >> preaim;
	fin >> Word >> aimspeed_uses_dst;
	fin >> Word >> aimspeed;
	fin >> Word >> aimheight;
	fin >> Word >> crosshair;
	fin >> Word >> autoshoot;
	fin >> Word >> as_xhairdst;
	fin >> Word >> asdelay;
	fin.close();
}

/*
#include <D3Dcompiler.h> //generateshader
#pragma comment(lib, "D3dcompiler.lib")
//generate shader func
HRESULT GenerateShader(ID3D11Device * pDevice, ID3D11PixelShader **pShader, float r, float g, float b)
{
	char szCast[] = "struct VS_OUT"
		"{"
		" float4 Position : SV_Position;"
		" float4 Color : COLOR0;"
		"};"

		"float4 main( VS_OUT input ) : SV_Target"
		"{"
		" float4 col;"
		" col.a = 1.0f;"
		" col.r = %f;"
		" col.g = %f;"
		" col.b = %f;"
		" return col;"
		"}";

	ID3D10Blob* pBlob;
	char szPixelShader[1000];

	sprintf_s(szPixelShader, szCast, r, g, b);

	ID3DBlob* error;

	HRESULT hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", NULL, NULL, "main", "ps_4_0", NULL, NULL, &pBlob, &error);

	if (FAILED(hr))
		return hr;

	hr = pDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pShader);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

ID3D11Texture2D* texc = nullptr;
ID3D11ShaderResourceView* textureColor;
void GenerateTexture(uint32_t pixelcolor, DXGI_FORMAT format)//DXGI_FORMAT_R32G32B32A32_FLOAT DXGI_FORMAT_R8G8B8A8_UNORM
{
	//static const uint32_t pixelcolor = 0xff00ff00; //0xff00ff00 green, 0xffff0000 blue, 0xff0000ff red
	D3D11_SUBRESOURCE_DATA initData = { &pixelcolor, sizeof(uint32_t), 0 };

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	pDevice->CreateTexture2D(&desc, &initData, &texc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srdes;
	memset(&srdes, 0, sizeof(srdes));
	srdes.Format = format;
	srdes.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srdes.Texture2D.MostDetailedMip = 0;
	srdes.Texture2D.MipLevels = 1;
	pDevice->CreateShaderResourceView(texc, &srdes, &textureColor);
}
*/

ImVec2 halve(const ImVec2& im_vec2) { return ImVec2(im_vec2.x * 0.5f, im_vec2.y * 0.5f); }

ImVec2 imvec2_plus(const ImVec2& left, const ImVec2& right) { return ImVec2(left.x + right.x, left.y + right.y); }
ImVec2 imvec2_minus(const ImVec2& left, const ImVec2& right) { return ImVec2(left.x - right.x, left.y - right.y); }

void DrawCrosshair() {
	// 
	constexpr float crosshair_circle_diameter = 32.f;
	const ImVec2 crosshair_circle_center = halve(ImGui::GetIO().DisplaySize);

	constexpr float crosshair_thickness = 1.8f;
	const float crosshair_circle_radius_outline = crosshair_circle_diameter * 0.5f;
	const ImU32 crosshair_circle_color_outline = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 0.f, 0.f, 1.f));

	ImGui::GetStyle().WindowPadding = ImVec2(0, 0);
	ImGui::GetStyle().WindowRounding = 0;
	ImGui::GetStyle().WindowBorderSize = 0.0f;

	constexpr float crosshair_lines_size = crosshair_circle_diameter * 1.45f;
	constexpr float crosshair_lines_half_size = crosshair_lines_size * 0.5f;

	const ImVec2 crosshair_circle_window_size = ImVec2(crosshair_lines_size, crosshair_lines_size);
	const ImVec2 crosshair_circle_window_position =
		imvec2_minus(crosshair_circle_center, halve(crosshair_circle_window_size));

	ImGui::SetNextWindowPos(crosshair_circle_window_position);
	ImGui::SetNextWindowSize(crosshair_circle_window_size);

	ImGui::Begin("crosshair_circle",
		nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	// drawList->AddCircleFilled(_reticleCenter, _reticleRadius, _reticleColor, 12);
	drawList->AddCircle(crosshair_circle_center,
		crosshair_circle_radius_outline,
		crosshair_circle_color_outline,
		14,
		crosshair_thickness);
	drawList->AddLine(
		imvec2_minus(crosshair_circle_center, ImVec2(crosshair_lines_half_size, (crosshair_thickness * 0.5f))),
		imvec2_plus(crosshair_circle_center, ImVec2(crosshair_lines_half_size, -(crosshair_thickness * 0.5f))),
		crosshair_circle_color_outline,
		crosshair_thickness);

	drawList->AddLine(
		imvec2_minus(crosshair_circle_center, ImVec2((crosshair_thickness * 0.5f), crosshair_lines_half_size)),
		imvec2_plus(crosshair_circle_center, ImVec2(-(crosshair_thickness * 0.5f), crosshair_lines_half_size)),
		crosshair_circle_color_outline,
		crosshair_thickness);

	ImGui::End();
}

void AimAtPos(float x, float y)
{
	float TargetX = 0;
	float TargetY = 0;

	//X Axis
	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	//Y Axis
	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}


	if (aimspeed > 0)
	{
		//dont move mouse more than 50 pixels at time (ghetto HFR)
		float dirX = TargetX > 0 ? 1.0f : -1.0f;
		float dirY = TargetY > 0 ? 1.0f : -1.0f;
		TargetX = dirX * fmin(aimspeed, abs(TargetX));
		TargetY = dirY * fmin(aimspeed, abs(TargetY));
		mouse_event(MOUSEEVENTF_MOVE, (float)TargetX, (float)TargetY, NULL, NULL);
	}
	else
		if (TargetX != 0 && TargetY != 0)
			mouse_event(MOUSEEVENTF_MOVE, (float)TargetX, (float)TargetY, NULL, NULL);
}

//==========================================================================================================================

//w2s stuff
struct vec2
{
	float x, y;
};

struct vec3
{
	float x, y, z;
};

struct vec4
{
	float x, y, z, w;
};

void MapBuffer(ID3D11Buffer * pStageBuffer, void** ppData, UINT * pByteWidth)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	HRESULT res = pContext->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes);

	D3D11_BUFFER_DESC desc;
	pStageBuffer->GetDesc(&desc);

	if (FAILED(res))
	{
		//Log("Map stage buffer failed {%d} {%d} {%d} {%d} {%d}", (void*)pStageBuffer, desc.ByteWidth, desc.BindFlags, desc.CPUAccessFlags, desc.Usage);
		SAFE_RELEASE(pStageBuffer); return;
	}
	
	*ppData = subRes.pData;
	
	if (pByteWidth)
		*pByteWidth = desc.ByteWidth;
}

void UnmapBuffer(ID3D11Buffer * pStageBuffer)
{
	pContext->Unmap(pStageBuffer, 0);
}

ID3D11Buffer* pStageBufferA = NULL;
ID3D11Buffer* CopyBufferToCpuA(ID3D11Buffer * pBufferA)
{
	D3D11_BUFFER_DESC CBDescA;
	pBufferA->GetDesc(&CBDescA);

	if (pStageBufferA == NULL)
	{
		//Log("onceA");
		// create buffer
		D3D11_BUFFER_DESC descA;
		descA.BindFlags = 0;
		descA.ByteWidth = CBDescA.ByteWidth;
		descA.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descA.MiscFlags = 0;
		descA.StructureByteStride = 0;
		descA.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDevice->CreateBuffer(&descA, NULL, &pStageBufferA)))
		{
			//Log("CreateBuffer failed when CopyBufferToCpuA {}");
		}
	}

	if (pStageBufferA != NULL)
		pContext->CopyResource(pStageBufferA, pBufferA);

	return pStageBufferA;
}

ID3D11Buffer* pStageBufferB = NULL;
ID3D11Buffer* CopyBufferToCpuB(ID3D11Buffer * pBufferB)
{
	D3D11_BUFFER_DESC CBDescB;
	pBufferB->GetDesc(&CBDescB);

	if (pStageBufferB == NULL)
	{
		//Log("onceB");
		// create buffer
		D3D11_BUFFER_DESC descB;
		descB.BindFlags = 0;
		descB.ByteWidth = CBDescB.ByteWidth;
		descB.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descB.MiscFlags = 0;
		descB.StructureByteStride = 0;
		descB.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDevice->CreateBuffer(&descB, NULL, &pStageBufferB)))
		{
			//Log("CreateBuffer failed when CopyBufferToCpuB {}");
		}
	}

	if (pStageBufferB != NULL)
		pContext->CopyResource(pStageBufferB, pBufferB);

	return pStageBufferB;
}

//get distance
float GetDst(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct AimEspInfo_t
{
	float vOutX, vOutY, vOutZ;
	float CrosshairDst;
};
std::vector<AimEspInfo_t>AimEspInfo;


//w2s
int WorldViewCBnum = 3; //2
int ProjCBnum = 4;//1
int matProjnum = 68;//16

ID3D11Buffer* pWorldViewCB = nullptr;
ID3D11Buffer* pProjCB = nullptr;

ID3D11Buffer* m_pCurWorldViewCB = NULL;
ID3D11Buffer* m_pCurProjCB = NULL;

float matWorldView[4][4];
float matProj[4][4];

float* worldview;
float* proj;

void AddModel(ID3D11DeviceContext * pContext)
{
	pContext->VSGetConstantBuffers(WorldViewCBnum, 1, &pWorldViewCB);	//WorldViewCBnum
	pContext->VSGetConstantBuffers(ProjCBnum, 1, &pProjCB);				//ProjCBnum

	if (pWorldViewCB == nullptr)
	{
		SAFE_RELEASE(pWorldViewCB); return;
	}
	if (pProjCB == nullptr)
	{
		SAFE_RELEASE(pProjCB); return;
	}

	if (pWorldViewCB != nullptr && pProjCB != nullptr)
	{
		m_pCurWorldViewCB = CopyBufferToCpuA(pWorldViewCB);
		m_pCurProjCB = CopyBufferToCpuB(pProjCB);
	}

	if (m_pCurWorldViewCB == nullptr)
	{
		SAFE_RELEASE(m_pCurWorldViewCB); return;
	}
	if (m_pCurProjCB == nullptr)
	{
		SAFE_RELEASE(m_pCurProjCB); return;
	}
	
	if (m_pCurWorldViewCB != nullptr && m_pCurProjCB != nullptr)
	{
		MapBuffer(m_pCurWorldViewCB, (void**)&worldview, NULL);
		memcpy(matWorldView, &worldview[0], sizeof(matWorldView));
		UnmapBuffer(m_pCurWorldViewCB);
		MapBuffer(m_pCurProjCB, (void**)&proj, NULL);
		memcpy(matProj, &proj[matProjnum], sizeof(matProj));			//matProjnum
		UnmapBuffer(m_pCurProjCB);
	}

	//TAB worldtoscreen unity 2018
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, aimheight, preaim, 1.0);
	DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);//multipication order matters

	//normal
	DirectX::XMMATRIX WorldViewProj = viewProjMatrix; //normal

	float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
	float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
	float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
	float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

	if (mw > 1.0f)
	{
		float invw = 1.0f / mw;
		mx *= invw;
		my *= invw;

		float x = ViewportWidth / 2.0f;
		float y = ViewportHeight / 2.0f;

		x += 0.5f * mx * ViewportWidth + 0.5f;
		y += 0.5f * my * ViewportHeight + 0.5f;//-
		mx = x;
		my = y;
	}
	else
	{
		mx = -1.0f;
		my = -1.0f;
	}
	AimEspInfo_t pAimEspInfo = { static_cast<float>(mx), static_cast<float>(my), static_cast<float>(mw) };
	AimEspInfo.push_back(pAimEspInfo);
}


