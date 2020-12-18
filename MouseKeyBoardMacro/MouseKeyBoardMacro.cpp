#include <iostream>
#include <string>
#include <array>
#include <bit>
#include <vector>
#include <queue>
#include <algorithm>
#include <span>

#define WIN32_LEAN_AND_MEAN        
#include <windows.h>

#include "../../../include/leikaifeng.h"

class CreateWindowHandle {

    static void _CreateWindowClass(HINSTANCE moduleHandle, LPCWSTR windowsClassName) {
	    WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = DefWindowProcW;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = moduleHandle;
        wcex.hIcon          = nullptr;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName   = nullptr;
        wcex.lpszClassName  = windowsClassName;
        wcex.hIconSm        = nullptr;

        if (FALSE == RegisterClassExW(&wcex)) {
            Exit("create window class error");
        }
    }

    static auto _CreateWindow(HINSTANCE moduleHandle, LPCWSTR windowsClassName) {
       
        auto windowsHandle = CreateWindowExW(0L, windowsClassName, L"Window", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, moduleHandle, nullptr);

        if (windowsHandle == FALSE) {
            Exit("create window error");
        }

        return windowsHandle;
    }

    HWND m_windowHandle;

public:
    CreateWindowHandle() {
        auto moduleHandle = static_cast<HINSTANCE>(GetModuleHandleW(nullptr));
      
        WCHAR windowsClassName[] = L"fsdfsrewrwegfdgfd";

        CreateWindowHandle::_CreateWindowClass(moduleHandle, windowsClassName);

        m_windowHandle = CreateWindowHandle::_CreateWindow(moduleHandle, windowsClassName);
    }

    auto GetHandle() {
        return m_windowHandle;
    }
};

void CreateMouseRawInput(HWND handle) {

    RAWINPUTDEVICE rid;

    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid.hwndTarget = handle;
    
    if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE) {
        Exit("create mouse raw input error");
    }
}

void CreateKeyboardRawInput(HWND handle) {

    RAWINPUTDEVICE rid;

    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid.hwndTarget = handle;

    if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE) {
        Exit("create keyboard raw input error");
    }
}

enum class InputFlag : unsigned char
{
    Down = 0x00,
  
    Up = KEYEVENTF_KEYUP
};


enum class VKCode : unsigned char {

    MouseWheel = 0x00,

    MouseLeft = 0x01,

    MouseRight = 0x02,

    MouseMiddle = 0x04,

    H = 0x48,

    C = 0x43,

    R = 0x52,

    G = 0x47,

    RightShift = VK_RSHIFT,
    
    LeftShift = VK_LSHIFT,

    Shift = 16,

    ArrayUp = 0x26,
    
    ArrayDown = 40,
    
    ArrayLeft = 37,
    
    ArrayRight = 39,

    P = 0x50,

    Q = 81,

    N1 = 49,

    N2 = 50,
};


class Input {

    static_assert(
        std::endian::native == std::endian::little &&
        std::is_same_v<std::underlying_type_t<InputFlag>, unsigned char> &&
        std::is_same_v<std::underlying_type_t<VKCode>, unsigned char>, "error");
    using NT = uint16_t;

    constexpr static NT OFFSET = sizeof(NT) * 8 / 2;

    NT m_value;

public:
    Input() :m_value(0) {}


    Input(InputFlag flag, VKCode code) {

        auto value = static_cast<NT>(flag);

        value <<= OFFSET;

        value |= static_cast<NT>(code);

        m_value = value;

    }

    NT GetValue() {
        return m_value;
    }
};

class LinkMap {

    
    template <typename T, size_t SIZE>
    requires(std::is_default_constructible_v<T>&& std::is_trivially_copy_assignable_v<T>&& SIZE != 0)
        class BufFix {

        constexpr static size_t LENGTH = SIZE * 4;

        constexpr static size_t ONE_BLACK_BYTES_SIZE = sizeof(T) * SIZE;

        constexpr static size_t MOVE_LENGTH = LENGTH - SIZE;

        T m_buffer[LENGTH];

        size_t m_index;

    public:
        BufFix() : m_buffer(), m_index(MOVE_LENGTH) {}

        void Add(T value) {

            m_index--;
            m_buffer[m_index] = value;
           
            if (m_index == 0) {

                std::memcpy(m_buffer + MOVE_LENGTH, m_buffer, ONE_BLACK_BYTES_SIZE);

                m_index = MOVE_LENGTH;
            }
            else {
                
            }
        }

        //必须保证std::vector的长度小于SIZE
        bool Cmp(std::span<Input>& value) {

            auto data = value.data();

            auto data_bytes_size = value.size() * sizeof(T);

            auto buffer = reinterpret_cast<char*>(&m_buffer[m_index]);

            return 0 == std::memcmp(buffer, data, data_bytes_size);
        }
    };

    constexpr static size_t SIZE = 8;

    using FT = std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>;

    using ET = std::pair<std::span<Input>, std::span<INPUT>>;
    

    std::vector<Input> m_key_source;
    std::vector<INPUT> m_value_source;
    
    std::vector<FT> m_firstNodes;
    std::vector<ET> m_nodes;

    BufFix<Input, SIZE> m_keys;


    void SendMacro(std::span<INPUT>& item) {
        SendInput(static_cast<UINT>(item.size()), item.data(), sizeof(INPUT));
    }
public:
    LinkMap() : m_value_source(), m_key_source(), m_nodes(), m_keys(), m_firstNodes() {
      
    }

    std::pair<size_t, size_t> CreateKey(const std::vector<Input>& key)
    {
        if (key.size() > SIZE) {
            Exit("key item too long");
        }


        auto offset = m_key_source.size();

        for (auto item = key.crbegin(); item != key.crend(); item++)
        {


            m_key_source.push_back(*item);
        }

        return std::make_pair(offset, key.size()); 
    }

    std::span<Input> CreateKey(const std::pair<size_t, size_t>& n) {
        auto item = m_key_source.begin();

        return std::span<Input>{item + n.first, n.second};
    }

    std::span<INPUT> CreateValue(const std::pair<size_t, size_t>& n) {
        auto item = m_value_source.begin();

        return std::span<INPUT>{item + n.first, n.second};
    }

    std::pair<size_t, size_t> CreateValue(const std::vector<INPUT>& value)
    {
        auto offset = m_value_source.size();

        for (auto& item : value)
        {


            m_value_source.push_back(item);
        }

        return std::make_pair(offset, value.size());
    }

    void Add(const std::vector<Input>& key, const std::vector<INPUT>& value) {
       
        

        m_firstNodes.push_back(std::make_pair(CreateKey(key), CreateValue(value)));


    }

    void Complete() {
        for (auto& item : m_firstNodes)
        {
            m_nodes.push_back(std::make_pair(CreateKey(item.first), CreateValue(item.second)));
        }
    }

    void Send(Input key) {

        m_keys.Add(key);


        for (auto& node : m_nodes) {
         
            if (m_keys.Cmp(node.first)) {

                SendMacro(node.second);

            }
        }
    }

    
};

auto GetScanCode(VKCode code) {
    auto value = MapVirtualKeyW(static_cast<UINT>(code), MAPVK_VK_TO_VSC);

    if (value == 0) {
        Exit("get scan code error");
    }
    else {
        return value;
    }
}



INPUT CreateKeyBoardInput(InputFlag flag, VKCode code) {


    KEYBDINPUT keyInput = {};

    keyInput.wVk = static_cast<WORD>(code);
   
    keyInput.wScan = static_cast<WORD>(GetScanCode(code));

    keyInput.dwFlags = static_cast<DWORD>(flag);


    keyInput.time = 0;
    
    keyInput.dwExtraInfo = 0;
   

    INPUT input = {};

    input.type = INPUT_KEYBOARD;

    input.ki = keyInput;

    return input;

}


INPUT CreareMouseInput(InputFlag flag, VKCode code) {

    MOUSEINPUT mouseInput = {};

    if (code == VKCode::MouseLeft) {
        if (flag == InputFlag::Down)
        {
            mouseInput.dwFlags = MOUSEEVENTF_LEFTDOWN;
        }
        else {
            mouseInput.dwFlags = MOUSEEVENTF_LEFTUP;
        }
    }

    INPUT input = {};


    input.type = INPUT_MOUSE;

    input.mi = mouseInput;

    return input;
}

class Info {

public:
    static auto& GetMouseData() {
        static LinkMap data{};
        return data;
    }

    static auto& GetKeyBoardData() {
        static LinkMap data{};

        return data;
    }

};




void MouseMacro(Input input) {

    Info::GetMouseData().Send(input);

}


void KeyBoardMacro(Input input) {

    Info::GetKeyBoardData().Send(input);
}


void KeyboardRawInput(RAWKEYBOARD& data) {
   
   
    if ((data.Flags & RI_KEY_BREAK) == RI_KEY_MAKE) {
       
        
        KeyBoardMacro(Input{ InputFlag::Down, static_cast<VKCode>(data.VKey) });

    }
    else if ((data.Flags & RI_KEY_BREAK) == RI_KEY_BREAK) {
      
        KeyBoardMacro(Input{ InputFlag::Up, static_cast<VKCode>(data.VKey) });

    }
    else {
        
    }
}


void MouseRawInput(RAWMOUSE& data) {
    
    if (data.usButtonFlags == 0) {

    }
    else if (data.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN) {
       
        MouseMacro(Input{ InputFlag::Down, VKCode::MouseLeft });
    }
    else if (data.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP) {
     
        MouseMacro(Input{ InputFlag::Up, VKCode::MouseLeft });
    }
    else if (data.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN) {
        
        MouseMacro(Input{ InputFlag::Down, VKCode::MouseMiddle });
    }
    else if (data.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP) {

        MouseMacro(Input{ InputFlag::Up, VKCode::MouseMiddle });
    }
    else if (data.usButtonFlags == RI_MOUSE_WHEEL) {

        if (data.usButtonData == 120) {

            MouseMacro(Input{ InputFlag::Up, VKCode::MouseWheel });
        }
        else {

            MouseMacro(Input{ InputFlag::Down, VKCode::MouseWheel });
        }
    }
    else if (data.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN) {
      
        MouseMacro(Input{ InputFlag::Down, VKCode::MouseRight });
    }
    else if (data.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP) {
       
        MouseMacro(Input{ InputFlag::Up, VKCode::MouseRight });
    }
    
}

template<UINT SIZE>
void frowRawInput(std::array<char, SIZE>& buffer, LPARAM lParam) {

    UINT dwSize = SIZE;

    if (-1 == GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER))) {
        Exit("get raw input data error");
    }
   
    auto raw = reinterpret_cast<RAWINPUT*>(buffer.data());

    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
        KeyboardRawInput(raw->data.keyboard);
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        MouseRawInput(raw->data.mouse);
    }
    else {
        Exit("other raw input message");
    }
}

void AddMouseData(std::vector<Input> key, std::vector<INPUT> value) {
    Info::GetMouseData().Add(key, value);
}

void AddKeyBoardData(std::vector<Input> key, std::vector<INPUT> value) {
    Info::GetKeyBoardData().Add(key, value);
}

int Start() {

    Info::GetKeyBoardData().Complete();

    Info::GetMouseData().Complete();

    CreateWindowHandle window{};


    CreateMouseRawInput(window.GetHandle());

    CreateKeyboardRawInput(window.GetHandle());


    std::array<char, 1024> buffer{};

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_INPUT) {

            frowRawInput(buffer, msg.lParam);

            if (GET_RAWINPUT_CODE_WPARAM(msg.wParam) == RIM_INPUT) {
                DispatchMessage(&msg);
            }
            else {

            }

        }
        else {
            DispatchMessage(&msg);
        }   
    }

    return (int)msg.wParam;
}


int main() {
    
    auto n1 = {
            CreateKeyBoardInput(InputFlag::Down, VKCode::N1),
            CreateKeyBoardInput(InputFlag::Up, VKCode::N1),
    };

    auto n2 = {
            CreateKeyBoardInput(InputFlag::Down, VKCode::N2),
            CreateKeyBoardInput(InputFlag::Up, VKCode::N2),
    };

    AddMouseData({
        Input{InputFlag::Up, VKCode::MouseLeft},
        Input{InputFlag::Down, VKCode::MouseRight},
        },
        n1
        );

    AddMouseData({
        Input{InputFlag::Up, VKCode::MouseLeft},
        Input{InputFlag::Down, VKCode::MouseRight},
        Input{InputFlag::Up, VKCode::MouseRight},
        },
        n1
        );

    AddMouseData({
        Input{InputFlag::Up, VKCode::MouseLeft},
        Input{InputFlag::Up, VKCode::MouseRight},
        },
        n1
        );

    AddMouseData({
        Input{InputFlag::Up, VKCode::MouseRight},
        Input{InputFlag::Down, VKCode::MouseLeft},
        Input{InputFlag::Up, VKCode::MouseLeft},
        },
        n2);

    AddMouseData({
       Input{InputFlag::Down, VKCode::MouseRight},
       Input{InputFlag::Down, VKCode::MouseLeft},
       Input{InputFlag::Up, VKCode::MouseLeft},
        },
        n2);

    AddMouseData({
        
        Input{InputFlag::Down, VKCode::MouseLeft},
        Input{InputFlag::Up, VKCode::MouseRight},
        Input{InputFlag::Up, VKCode::MouseLeft},
        },
        n2);

    AddMouseData({
        Input{InputFlag::Down, VKCode::MouseMiddle},
        },
        {
            CreateKeyBoardInput(InputFlag::Down, VKCode::H),
            CreateKeyBoardInput(InputFlag::Up, VKCode::H),
            
        });

    AddMouseData({
       Input{InputFlag::Up, VKCode::MouseMiddle},
        },
        {
            CreateKeyBoardInput(InputFlag::Down, VKCode::H),
            CreateKeyBoardInput(InputFlag::Up, VKCode::H),
            CreateKeyBoardInput(InputFlag::Down, VKCode::C),
            CreateKeyBoardInput(InputFlag::Up, VKCode::C),
            CreateKeyBoardInput(InputFlag::Down, VKCode::H),
            CreateKeyBoardInput(InputFlag::Up, VKCode::H),
        });

    
    AddMouseData({
        Input{InputFlag::Down, VKCode::MouseMiddle},
        Input{InputFlag::Down, VKCode::MouseLeft},
        },
        {
            CreateKeyBoardInput(InputFlag::Down, VKCode::R),
            CreateKeyBoardInput(InputFlag::Up, VKCode::R),
        });



    auto down = {
        CreateKeyBoardInput(InputFlag::Down, VKCode::P),
        CreateKeyBoardInput(InputFlag::Down, VKCode::C),
    };

    auto up = {
        CreateKeyBoardInput(InputFlag::Up, VKCode::C),
        CreateKeyBoardInput(InputFlag::Up, VKCode::P),   
    };

    AddKeyBoardData({

        Input{InputFlag::Down, VKCode::ArrayUp},
        Input{InputFlag::Up, VKCode::ArrayUp},
        Input{InputFlag::Down, VKCode::ArrayUp},
       
        },
        down);

    AddKeyBoardData({

       Input{InputFlag::Down, VKCode::ArrayUp},
       Input{InputFlag::Down, VKCode::ArrayUp},
       Input{InputFlag::Up, VKCode::ArrayUp},

        },
        up);


    AddKeyBoardData({

      Input{InputFlag::Down, VKCode::ArrayDown},
      Input{InputFlag::Up, VKCode::ArrayDown},

        },
        up);

    AddKeyBoardData({

      Input{InputFlag::Down, VKCode::ArrayLeft},
      Input{InputFlag::Up, VKCode::ArrayLeft},

        },
        up);

    AddKeyBoardData({

      Input{InputFlag::Down, VKCode::ArrayRight},
      Input{InputFlag::Up, VKCode::ArrayRight},

        },
        up);


    
    return Start();
}