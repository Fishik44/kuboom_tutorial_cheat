#include "includes.h"
#include "render.h"

uintptr_t GameAssembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll");

auto get_transform = reinterpret_cast<uintptr_t(*)(uintptr_t)>(reinterpret_cast<LPVOID>(GameAssembly + 0x6FC1F0));
auto camera_main = reinterpret_cast<uintptr_t(*)()>(reinterpret_cast<LPVOID>(GameAssembly + 0x6F92C0));
auto WorldToScreenPoint = reinterpret_cast<Vector3(*)(uintptr_t, Vector3)>(reinterpret_cast<LPVOID>(GameAssembly + 0x6F8E70));
auto get_position = reinterpret_cast<Vector3(*)(uintptr_t)>(reinterpret_cast<LPVOID>(GameAssembly + 0xBAAB10));

auto domain = reinterpret_cast<uintptr_t(*)()>(reinterpret_cast<LPVOID>(GetProcAddress((HMODULE)GameAssembly, "il2cpp_domain_get")));
auto thread_attach = reinterpret_cast<uintptr_t(*)(uintptr_t)>(reinterpret_cast<LPVOID>(GetProcAddress((HMODULE)GameAssembly, "il2cpp_thread_attach")));
auto thread_detach = reinterpret_cast<uintptr_t(*)(uintptr_t)>(reinterpret_cast<LPVOID>(GetProcAddress((HMODULE)GameAssembly, "il2cpp_thread_detach")));

c_renderer render;

void aaaa() {
	auto Kube_s = *(uintptr_t*)(*(uintptr_t*)(GameAssembly + 0x11D05D4) + 0x5C);
	if (!Kube_s)
		return;

	uintptr_t bcs = *(uintptr_t*)(Kube_s + 0x1C);
	if (!bcs)
		return;

	uintptr_t ps = *(uintptr_t*)(bcs + 0x7C);
	if (!ps)
		return;

	uintptr_t list = *(uintptr_t*)(bcs + 0x1C4);
	if (!list)
		return;

	int _size = *(int*)((uintptr_t)list + 0xC);

	uintptr_t _items = *(uintptr_t*)(list + 0x8);
	if (!_items)
		return;

	uintptr_t* Item = (uintptr_t*)((uintptr_t)_items + 0x10);
	if (!Item)
		return;

	auto size = ImGui::GetIO().DisplaySize;

	for (int i = 0; i < _size; i++) {

		uintptr_t players = Item[i];
		if (!players)
			continue;

		if (*(uintptr_t*)(players + 0x40) != ps) {

			if (*(int*)(players + 0x30) != *(int*)(ps + 0x1A8)) {

				uintptr_t camera = camera_main();
				if (!camera)
					continue;

				uintptr_t ps = *(uintptr_t*)(players + 0x40);
				if (!ps)
					continue;

				auto transform = get_transform(ps);
				if (!transform)
					continue;

				Vector3 oPisition = get_position(transform);

				Vector3 position = WorldToScreenPoint(camera, oPisition);
				Vector3 head = WorldToScreenPoint(camera, { oPisition.x, oPisition.y + 1.9f, oPisition.z });

				position.y = size.y - position.y;
				head.y = size.y - head.y;

				render.draw_cornerbox(position, head, 2.5f, ImColor(1.f, 1.f, 1.f), true);

			}

		}

	}
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	uintptr_t thread = thread_attach(domain());

	aaaa();

	thread_detach(thread);

	ImGui::Begin("ImGui Window");

	ImGui::End();

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)& oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}