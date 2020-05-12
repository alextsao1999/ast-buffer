//
// Created by Alex on 2020/5/8.
//

#pragma once
#include<Windows.h>
#include<d2d1.h>
#include<dwrite.h>
#include<string>
#include<vector>
#include<map>
#include <ast_buffer.h>
#include <origin.h>
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"winmm.lib")

std::map<std::string, int> color_table = {
        {"black", D2D1::ColorF::Black},
        {"red", D2D1::ColorF::DarkRed},
        {"blue", D2D1::ColorF::DarkBlue},
        {"green", D2D1::ColorF::DarkSeaGreen},
        {"purple", D2D1::ColorF::Purple},
};

ts::Query query = ts::Language::cpp().query("(number_literal) @purple"
                                            "(string_literal) @green"
                                            "(primitive_type) @blue"
                                            "(comment) @red"
                                            );

void start();
int main() {
    start();
    return 0;
}

//方便释放COM资源
template <typename InterfaceType>
inline void SafeRelease(InterfaceType** currentObject)
{
    if (*currentObject != nullptr)
    {
        (*currentObject)->Release();
        *currentObject = nullptr;
    }
}


//自定义的窗口类
class MyWindow
{
public:
    MyWindow();
    ~MyWindow();

    void init();

    void createWindow(const std::wstring& windowTitle, int left, int top, int width, int height);

    void run();

    //窗口事件，由子类负责实现
    virtual void onResize(UINT32 width, UINT32 height) {}
    virtual void onPaint() {}
    virtual void onMouseEvent(UINT msg, WPARAM wp, LPARAM lp) {}
    virtual void onScroll(short delta) {}
    virtual void onKey(UINT32 vk) {}
    virtual void onChar(UINT32 c) {}
protected:
    //使用字符串创建用于绘制的文本布局
    IDWriteTextLayout * createTextLayout(const std::wstring &text) {
        IDWriteTextLayout* textLayout = nullptr;

        if (!textFormat) {
            dwriteFactory->CreateTextFormat(L"", 0,
                                            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"", &textFormat);
        }

        dwriteFactory->CreateTextLayout(
                text.c_str(),
                static_cast<UINT32>(text.length()),
                textFormat,
                width, height,
                &textLayout
        );
        //SafeRelease<IDWriteTextFormat>(&textFormat);

        return textLayout;
    }


    //窗口宽、高
    UINT32 width;
    UINT32 height;

    ID2D1SolidColorBrush *brush;//画刷
    ID2D1HwndRenderTarget * target;//用于呈现绘制结果
    IDWriteTextFormat*textFormat = nullptr;
private:
    void registerWindow();
    void createD2DResource();

    LRESULT messageProc(UINT msg, WPARAM wp, LPARAM lp);

    bool isRunning;//窗口是否正在运行

    HWND hWnd;//窗口句柄

    //使用dwrite和direct2d不可缺少的
    ID2D1Factory * d2dFactory;
    IDWriteFactory* dwriteFactory;




};

//定义光标移动的方式
enum class SelectMode {
    head, tile,
    lastChar, nextChar,
    lastWord, nextWord,
    absoluteLeading, absoluteTrailing,
    lastLine, nextLine,
    all,
    up, down,
};

class Editor :
        public MyWindow {

public:
    Editor();
    ~Editor();

private:
    //重写基类函数以响应事件
    void onResize(UINT32 width, UINT32 height)override {
        needUpdate = true;
    }
    void onPaint()override;
    void onMouseEvent(UINT msg, WPARAM wp, LPARAM lp)override;
    void onScroll(short delta)override;
    void onKey(UINT32 vk)override;
    void onChar(UINT32 c)override;

    //使用指定方式移动光标
    void select(SelectMode mode, bool moveAnchor = true);

    //移动光标至指定点
    void setSelectionFromPoint(float x, float y, bool moveAnchor) {
        BOOL isTrailingHit;
        BOOL isInside;
        DWRITE_HIT_TEST_METRICS caretMetrics;


        textLayout->HitTestPoint(
                x, y,
                &isTrailingHit,
                &isInside,
                &caretMetrics
        );

        if (isTrailingHit) {
            caretPosition = caretMetrics.textPosition + caretMetrics.length;
        }
        else {
            caretPosition = caretMetrics.textPosition;
        }

        if (moveAnchor)
            caretAnchor = caretPosition;

        return;
    }

    //复制及粘贴
    void copyToClipboard() {
        DWRITE_TEXT_RANGE selectionRange = getSelectionRange();
        if (selectionRange.length <= 0)
            return;

        if (OpenClipboard(0)) {
            if (EmptyClipboard()) {

                size_t byteSize = sizeof(wchar_t) * (selectionRange.length + 1);
                HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);

                if (hClipboardData != NULL) {
                    void* memory = GlobalLock(hClipboardData);

                    if (memory != NULL) {
                        const wchar_t* ctext = text.buffer().range_string(0, text.length()).c_str();
                        memcpy(memory, &ctext[selectionRange.startPosition], byteSize);
                        GlobalUnlock(hClipboardData);

                        if (SetClipboardData(CF_UNICODETEXT, hClipboardData) != NULL) {
                            hClipboardData = NULL;
                        }
                    }
                    GlobalFree(hClipboardData);
                }
            }
            CloseClipboard();
        }
    }
    void pasteFromClipboard() {

        deleteSelection();

        UINT32 characterCount = 0;

        if (OpenClipboard(0)) {
            HGLOBAL hClipboardData = GetClipboardData(CF_UNICODETEXT);

            if (hClipboardData != NULL)
            {
                // Get text and size of text.
                size_t byteSize = GlobalSize(hClipboardData);
                void* memory = GlobalLock(hClipboardData); // [byteSize] in bytes
                const wchar_t* ctext = reinterpret_cast<const wchar_t*>(memory);
                characterCount = static_cast<UINT32>(wcsnlen(ctext, byteSize / sizeof(wchar_t)));

                if (memory != NULL)
                {
                    // Insert the text at the current position.
                    text.insert(
                            caretPosition,
                            std::wstring(ctext,
                                         characterCount)
                    );

                    GlobalUnlock(hClipboardData);

                }
            }
            CloseClipboard();
        }

        caretPosition += characterCount;
        caretAnchor = caretPosition;

        needUpdate = true;
    }

    //得到选中文本的范围
    DWRITE_TEXT_RANGE getSelectionRange();

    //删除选中文本
    void deleteSelection() {
        DWRITE_TEXT_RANGE range = getSelectionRange();

        if (range.length <= 0)
            return;

        text.erase(range.startPosition, range.startPosition + range.length);

        caretPosition = range.startPosition;
        caretAnchor = caretPosition;

        needUpdate = true;
    }

    //检查 及 更新文本布局
    void checkUpdate() {
        if (needUpdate) {
            IDWriteTextLayout *temp = createTextLayout(text.buffer().range_string(0, text.length()));
            if (temp) {
                needUpdate = false;
                SafeRelease(&textLayout);//释放上一个文本布局
                textLayout = temp;
            }
            //获取文本区域的宽高
            DWRITE_TEXT_METRICS metrics;
            textLayout->GetMetrics(&metrics);

            //修改更新后的滚动值
            maxScrollY = max(metrics.height - height, 0);
            if (scrollY > maxScrollY)
                scrollY = maxScrollY;
        }
    }

    //判断两个字符是否是一组
    bool isUnicodeUnit(wchar_t char1, wchar_t char2);

    //三个绘制函数
    void fillSelectedRange();
    void drawCaret();
    void drawText();

    //被编辑的字符串
    ASTBuffer<wchar_t> text = ts::Language::cpp();

    //需要更新文本布局的标记，每一帧都会检查这个值
    bool needUpdate;

    //Y轴最大滚动距离 及 当前滚动距离
    float maxScrollY;
    float scrollY;

    //编辑状态时需要 保持 光标可见，此变量用于 判断 当前是否为滚轮在滚动，
    bool isOnScroll;

    //光标所在位置 及 锚点位置，两者之间的文本处于选中状态
    UINT32 caretAnchor;
    UINT32 caretPosition;

    //呈现在窗口上的文本布局
    IDWriteTextLayout * textLayout;

    //在输入状态时，光标不应闪烁，
    //使用此变量以记录上次的输入时间，绘制时比较当前时间与其差值以决定是否闪烁
    float lastInputTime;

    //记录上次点击时间，用于进行双击的判断
    float lastClickTime;

    //鼠标双击时，文本会被全选，
    //但若当前选中不为空，则不进行全选
    UINT32 lastSelectLength;

};

void start() {
    Editor editor;
    editor.init();
    editor.createWindow(L"helloworld", 0, 0, 800, 500);
    editor.run();
}

//进程句柄宏定义
#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONET ((HINSTANCE)&__ImageBase)
#endif


MyWindow::MyWindow()
{
    hWnd = 0;
    width = 0;
    height = 0;

    isRunning = false;

    d2dFactory = nullptr;
    dwriteFactory = nullptr;
    target = nullptr;
    brush = nullptr;
}


MyWindow::~MyWindow()
{
    SafeRelease(&brush);
    SafeRelease(&target);
    SafeRelease(&dwriteFactory);
    SafeRelease(&d2dFactory);
    UnregisterClassW(L"myWindow", HINST_THISCOMPONET);
}

void MyWindow::init()
{
    registerWindow();
}

void MyWindow::createWindow(const std::wstring &windowTitle, int left, int top, int width, int height)
{
    if (!CreateWindowW(L"myWindow", windowTitle.c_str(), WS_OVERLAPPEDWINDOW,
                       left, top,
                       width, height,
                       0, 0, HINST_THISCOMPONET, this))
        throw std::exception("create window failed");

    createD2DResource();
}

void MyWindow::registerWindow()
{
    WNDCLASSW wc = { 0 };
    wc.hInstance = HINST_THISCOMPONET;
    wc.lpszClassName = L"myWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hIcon = LoadIconA(0, MAKEINTRESOURCEA(IDI_APPLICATION));
    wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(IDC_IBEAM));
    wc.cbWndExtra = sizeof(void*);
    wc.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)->LRESULT {
        MyWindow *window = static_cast<MyWindow*>(reinterpret_cast<void*>(GetWindowLongPtr(hWnd, 0)));
        if (window) {
            return window->messageProc(message, wParam, lParam);
        }
        else {
            if (message == WM_CREATE) {
                LPCREATESTRUCT cs = reinterpret_cast<LPCREATESTRUCT>(lParam);
                window = static_cast<MyWindow*>(cs->lpCreateParams);

                SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(window));
                window->hWnd = hWnd;
                return window->messageProc(message, wParam, lParam);
            }
            else return DefWindowProc(hWnd, message, wParam, lParam);
        }
    };
    if (!RegisterClassW(&wc))
        throw std::exception("register window failed");
}

void MyWindow::createD2DResource()
{
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory)))
        throw std::exception("create d2dFactory failed");

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwriteFactory)))
        throw std::exception("create dwrite factory failed");

    if (FAILED(d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                                  D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU((UINT32)0, (UINT32)0)), &target)))
        throw std::exception("create hwndTarget failed");

    //设置target的抗锯齿效果
    target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    //创建画刷用于后面的绘制，其颜色可以在后面更改
    if (FAILED(target->CreateSolidColorBrush(D2D1::ColorF(0xffffff, 1.0f), &brush)))
        throw std::exception("create brush failed");
}

LRESULT MyWindow::messageProc(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_KEYDOWN:
            onKey(static_cast<UINT>(wp));
            return 0;
        case WM_CHAR:
            onChar(static_cast<UINT>(wp));
            return 0;
        case WM_MOUSEWHEEL:
            onScroll(HIWORD(wp));
            return 0;
        case WM_LBUTTONDOWN:
            SetFocus(hWnd);
            SetCapture(hWnd);
            onMouseEvent(msg, wp, lp);
            return 0;
        case WM_LBUTTONUP:
            onMouseEvent(msg, wp, lp);
            ReleaseCapture();
            return 0;
        case WM_MOUSEMOVE:
            onMouseEvent(msg, wp, lp);
            return 0;
        case WM_PAINT:
            target->BeginDraw();
            target->Clear(D2D1::ColorF(D2D1::ColorF::White));
            onPaint();
            target->EndDraw();
            ValidateRect(hWnd, 0);
            return 0;
        case WM_ERASEBKGND:
            return 0;
        case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hWnd, &rect);
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
            if (target) {
                target->Resize(D2D1::SizeU(width, height));
            }
            onResize(width, height);
            return 0;
        }
        case WM_DESTROY:
            isRunning = false;
            return 0;
    }

    return DefWindowProc(hWnd, msg, wp, lp);

}

void MyWindow::run()
{
    //显示窗口
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg = { 0 };

    isRunning = true;

    while (isRunning) {
        if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {

            TranslateMessage(&msg);
            DispatchMessageW(&msg);

        }
        else {
            //hWndRenderTarget能自己控制帧率
            target->BeginDraw();
            target->Clear(D2D1::ColorF(D2D1::ColorF::White));
            onPaint();
            target->EndDraw();

        }

    }
}


namespace {
    inline bool IsHighSurrogate(UINT32 ch) throw()
    {
        // 0xD800 <= ch <= 0xDBFF
        return (ch & 0xFC00) == 0xD800;
    }

    inline bool IsLowSurrogate(UINT32 ch) throw()
    {
        // 0xDC00 <= ch <= 0xDFFF
        return (ch & 0xFC00) == 0xDC00;
    }
}

Editor::Editor()
{
    maxScrollY = 0.f;
    scrollY = 0.f;

    lastInputTime = -1.f;
    lastClickTime = -1.f;
    lastSelectLength = 0;

    isOnScroll = false;

    textLayout = nullptr;
    needUpdate = true;
    caretAnchor = 0;
    caretPosition = 0;

    text.append(L"//这就是一个单纯的测试文件\n"
                "//每一次更新都会重新构建所有文本的TextLayout \n"
                "//速度很慢\n"
                "//请酌情参考\n"
                "// 代码改自 : https://blog.csdn.net/qq_35679818/article/details/84448433\n"
                "int main() {\n"
                "    int int_literal = 0x9999;\n"
                "    auto *str = \"this is string_literal\";\n"
                "    return 0;\n"
                "}");
}

Editor::~Editor()
{
    SafeRelease(&textLayout);
}


void Editor::select(SelectMode mode, bool moveAnchor)
{
    //以下代码使用了dwrite的api

    switch (mode)
    {
        case SelectMode::up:
        case SelectMode::down:
        {
            std::vector<DWRITE_LINE_METRICS> lineMetrics;
            DWRITE_TEXT_METRICS textMetrics;
            textLayout->GetMetrics(&textMetrics);

            lineMetrics.resize(textMetrics.lineCount);
            textLayout->GetLineMetrics(&lineMetrics.front(), textMetrics.lineCount, &textMetrics.lineCount);

            UINT32 line = 0;
            UINT32 linePosition = 0;
            UINT32 nextLinePosition = 0;
            UINT32 lineCount = static_cast<UINT32>(lineMetrics.size());
            for (; line < lineCount; ++line)
            {
                linePosition = nextLinePosition;
                nextLinePosition = linePosition + lineMetrics[line].length;
                if (nextLinePosition > caretPosition) {
                    break;
                }
            }

            if (line > lineCount - 1) {
                line = lineCount - 1;
            }

            if (mode == SelectMode::up)
            {
                if (line <= 0)
                    break;
                line--;
                linePosition -= lineMetrics[line].length;
            }
            else
            {
                linePosition += lineMetrics[line].length;
                line++;
                if (line >= lineMetrics.size())
                    break;
            }

            DWRITE_HIT_TEST_METRICS hitTestMetrics;
            float caretX, caretY, dummyX;

            textLayout->HitTestTextPosition(
                    caretPosition,
                    false,
                    &caretX,
                    &caretY,
                    &hitTestMetrics
            );

            textLayout->HitTestTextPosition(
                    linePosition,
                    false,
                    &dummyX,
                    &caretY,
                    &hitTestMetrics
            );

            BOOL isInside, isTrailingHit;
            textLayout->HitTestPoint(
                    caretX,
                    caretY,
                    &isTrailingHit,
                    &isInside,
                    &hitTestMetrics
            );

            caretPosition = hitTestMetrics.textPosition;

            if (isTrailingHit) {
                caretPosition += hitTestMetrics.length;
            }
            break;
        }
        case SelectMode::head:
            caretPosition = 0;
            break;
        case SelectMode::tile:
            caretPosition = text.length();
            break;
        case SelectMode::lastChar:
            if (caretPosition > 0) {
                UINT32 moveCount = 1;

                if (caretPosition >= 2
                    && caretPosition <= text.length())
                {
                    if (isUnicodeUnit(text[caretPosition - 1], text[caretPosition - 2]))
                    {
                        moveCount = 2;
                    }
                }
                if (caretPosition < (UINT32)moveCount)
                    caretPosition = 0;
                else caretPosition -= moveCount;
            }
            break;
        case SelectMode::nextChar:
            if (caretPosition < text.length()) {
                UINT32 moveCount = 1;
                if (caretPosition >= 0
                    && caretPosition <= text.length() - 2)
                {
                    wchar_t charBackOne = text[caretPosition];
                    wchar_t charBackTwo = text[caretPosition + 1];
                    if (isUnicodeUnit(text[caretPosition], text[caretPosition + 1]))
                    {
                        moveCount = 2;
                    }
                }
                if (caretPosition > text.length())
                    caretPosition = text.length();
                else caretPosition += moveCount;
            }
            break;
        case SelectMode::lastWord:
        case SelectMode::nextWord: {
            std::vector<DWRITE_CLUSTER_METRICS> clusterMetrics;
            UINT32 clusterCount;
            textLayout->GetClusterMetrics(NULL, 0, &clusterCount);
            if (clusterCount == 0)
                break;

            clusterMetrics.resize(clusterCount);
            textLayout->GetClusterMetrics(&clusterMetrics.front(), clusterCount, &clusterCount);

            UINT32 clusterPosition = 0;
            UINT32 oldCaretPosition = caretPosition;

            if (mode == SelectMode::lastWord) {

                caretPosition = 0;
                for (UINT32 cluster = 0; cluster < clusterCount; ++cluster) {

                    clusterPosition += clusterMetrics[cluster].length;
                    if (clusterMetrics[cluster].canWrapLineAfter) {
                        if (clusterPosition >= oldCaretPosition)
                            break;

                        caretPosition = clusterPosition;
                    }

                }

            }
            else {
                for (UINT32 cluster = 0; cluster < clusterCount; ++cluster) {
                    UINT32 clusterLength = clusterMetrics[cluster].length;

                    if (clusterPosition + clusterMetrics[cluster].length > oldCaretPosition && clusterMetrics[cluster].canWrapLineAfter) {
                        caretPosition = clusterPosition + clusterMetrics[cluster].length;
                        break;

                    }
                    clusterPosition += clusterLength;
                    caretPosition = clusterPosition;
                }
            }
            break;
        }
        case SelectMode::absoluteLeading: {
            DWRITE_HIT_TEST_METRICS hitTestMetrics;
            float caretX, caretY;

            textLayout->HitTestTextPosition(
                    caretPosition,
                    false,
                    &caretX,
                    &caretY,
                    &hitTestMetrics
            );

            caretPosition = hitTestMetrics.textPosition;

            break;
        }
        case SelectMode::absoluteTrailing: {
            DWRITE_HIT_TEST_METRICS hitTestMetrics;
            float caretX, caretY;

            textLayout->HitTestTextPosition(
                    caretPosition,
                    true,
                    &caretX,
                    &caretY,
                    &hitTestMetrics
            );

            caretPosition = hitTestMetrics.textPosition + hitTestMetrics.length;
            break;
        }
        case SelectMode::all:
            caretAnchor = 0;
            caretPosition = text.length();
            return;
        default:
            break;
    }

    if (moveAnchor)
        caretAnchor = caretPosition;

}

void Editor::onPaint()
{
    checkUpdate();

    if (textLayout) {

        fillSelectedRange();

        drawCaret();

        drawText();

    }
}

void Editor::onMouseEvent(UINT msg, WPARAM wp, LPARAM lp)
{
    float x = (float)(short)LOWORD(lp);
    float y = (float)(short)HIWORD(lp);

    float time = timeGetTime() / 1000.f;

    const float doubleClickInterval = 0.3f;

    switch (msg)
    {
        case WM_LBUTTONDOWN:
            isOnScroll = false;
            lastSelectLength = getSelectionRange().length;
            setSelectionFromPoint(x, y + scrollY, (GetKeyState(VK_SHIFT) & 0x80) == 0);
            break;
        case WM_LBUTTONUP:
            if (time - lastClickTime < doubleClickInterval) {
                if (lastSelectLength == 0)
                    select(SelectMode::all);
            }
            lastClickTime = time;
            break;
        case WM_MOUSEMOVE:
            if ((wp & MK_LBUTTON) != 0)
                setSelectionFromPoint(x, y + scrollY, false);
            break;
        default:
            break;
    }
}

void Editor::onScroll(short delta)
{
    isOnScroll = true;

    //滚动事件发生不意味着滚动值改变

    float nextScroll = scrollY - delta;

    if (nextScroll < 0)
        nextScroll = 0;
    else if (nextScroll > maxScrollY)
        nextScroll = maxScrollY;

    if (nextScroll != scrollY) {
        scrollY = nextScroll;
        needUpdate = true;
    }

}


void Editor::onKey(UINT32 vk)
{
    bool heldShift = (GetKeyState(VK_SHIFT) & 0x80) != 0;
    bool heldControl = (GetKeyState(VK_CONTROL) & 0x80) != 0;
    bool heldAlt = (GetKeyState(VK_MENU) & 0x80) != 0;

    switch (vk)
    {
        case VK_RETURN:
            deleteSelection();
            wchar_t chars[3];
            chars[0] = '\n';
            chars[1] = 0;
            text.insert(caretPosition, std::wstring(chars, 1));

            caretPosition += 1;
            caretAnchor = caretPosition;

            needUpdate = true;
            break;
        case VK_BACK:

            if (caretPosition != caretAnchor)
            {
                deleteSelection();
            }
            else if (caretPosition > 0)
            {
                UINT32 count = 1;
                if (caretPosition >= 2
                    && caretPosition <= text.length())
                {
                    wchar_t charBackOne = text[caretPosition - 1];
                    wchar_t charBackTwo = text[caretPosition - 2];
                    if ((IsLowSurrogate(charBackOne) && IsHighSurrogate(charBackTwo))
                        || (charBackOne == '\n' && charBackTwo == '\r'))
                    {
                        count = 2;
                    }
                }

                caretPosition -= count;
                caretAnchor = caretPosition;

                text.erase(caretPosition, caretPosition + count);

                needUpdate = true;

            }
            break;
        case VK_DELETE:
            if (caretPosition != caretAnchor) {
                deleteSelection();
            }
            else {
                DWRITE_HIT_TEST_METRICS hitTestMetrics;
                float caretX, caretY;

                textLayout->HitTestTextPosition(
                        caretPosition,
                        false,
                        &caretX,
                        &caretY,
                        &hitTestMetrics
                );

                text.erase(hitTestMetrics.textPosition, hitTestMetrics.length);
                needUpdate = true;
            }

            break;
        case VK_TAB:
            break;
        case VK_LEFT:
            if (!heldControl)
                select(SelectMode::lastChar, !heldShift);
            else
                select(SelectMode::lastWord, !heldShift);
            break;

        case VK_RIGHT:
            if (!heldControl)
                select(SelectMode::nextChar, !heldShift);
            else
                select(SelectMode::nextWord, !heldShift);
            break;
        case VK_UP:
            select(SelectMode::up);
            break;
        case VK_DOWN:
            select(SelectMode::down);
            break;
        case VK_HOME:
            select(SelectMode::head);
            break;
        case VK_END:
            select(SelectMode::tile);
            break;
        case 'C':
            if (heldControl)
                copyToClipboard();
            break;
        case VK_INSERT:
            if (heldControl)
                copyToClipboard();
            else if (heldShift) {
                pasteFromClipboard();
            }
            break;
        case 'V':
            if (heldControl) {
                pasteFromClipboard();
            }
            break;
        case 'X':
            //剪切文本，先复制再删除
            if (heldControl) {
                copyToClipboard();
                deleteSelection();
            }
            break;
        case 'A':
            if (heldControl) {
                select(SelectMode::all);
            }
            break;
        default:
            return;
    }
    isOnScroll = false;
    lastInputTime = timeGetTime() / 1000.f;
}

void Editor::onChar(UINT32 c)
{
    if (c >= 0x20 || c == 9)
    {
        deleteSelection();

        UINT32 charsLength = 1;
        wchar_t chars[2] = { static_cast<wchar_t>(c), 0 };

        if (c > 0xFFFF)
        {
            chars[0] = wchar_t(0xD800 + (c >> 10) - (0x10000 >> 10));
            chars[1] = wchar_t(0xDC00 + (c & 0x3FF));
            charsLength++;
        }

        text.insert(caretPosition, std::wstring(chars, charsLength));

        caretPosition += charsLength;
        caretAnchor = caretPosition;

        needUpdate = true;
        isOnScroll = false;

        lastInputTime = timeGetTime() / 1000.f;
    }
    text.dump();

}

bool Editor::isUnicodeUnit(wchar_t char1, wchar_t char2)
{
    return (IsLowSurrogate(char1) && IsHighSurrogate(char2))
           || (char1 == '\n' && char2 == '\r');
}

void Editor::fillSelectedRange()
{
    UINT32 actualHitTestCount = 0;
    auto selectedRange = getSelectionRange();
    if (selectedRange.length > 0) {
        textLayout->HitTestTextRange(selectedRange.startPosition, selectedRange.length, 0, 0, 0, 0, &actualHitTestCount);
        std::vector<DWRITE_HIT_TEST_METRICS>hitTestMetrics(actualHitTestCount);
        textLayout->HitTestTextRange(selectedRange.startPosition, selectedRange.length, 0, -scrollY, &hitTestMetrics[0], static_cast<UINT32>(hitTestMetrics.size()), &actualHitTestCount);

        //改变画刷为天蓝色
        brush->SetColor(D2D1::ColorF(D2D1::ColorF::LightSkyBlue));

        //遍历选中区域并进行填充
        for (UINT32 i = 0; i < actualHitTestCount; i++) {
            const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
            D2D1_RECT_F highlightRect = {
                    htm.left,
                    htm.top,
                    (htm.left + htm.width),
                    (htm.top + htm.height)
            };
            target->FillRectangle(highlightRect, brush);
        }
    }
}

void Editor::drawCaret()
{
    DWRITE_HIT_TEST_METRICS caretMetrics;
    float caretX, caretY;
    textLayout->HitTestTextPosition(caretPosition, false, &caretX, &caretY, &caretMetrics);

    //若不处于滚动状态，则对光标位置进行判断修改，使其处于显示区域
    if (!isOnScroll) {
        if (caretY - scrollY + caretMetrics.height > height) {//光标超出窗口底部
            scrollY = caretY - height + caretMetrics.height;
        }
        else if (caretY - scrollY < 0) {//光标在窗口上方
            scrollY = caretY;
        }
    }

    //使用sin函数决定是否绘制caret
    if (sin((timeGetTime() / 1000.f - lastInputTime)*6.28f) > -0.1) {

        //caret颜色为黑色
        brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));

        target->DrawLine(D2D1::Point2F(caretX, caretY - scrollY)
                , D2D1::Point2F(caretX, caretY + caretMetrics.height - scrollY), brush);

    }
}

void Editor::drawText()
{
    //文本为黑色
    brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
    brush->SetOpacity(150);
    target->DrawTextLayout(D2D1::Point2F(0, -scrollY), textLayout, brush);

    auto cursor = query.exec(text.tree().root());
    uint32_t  index;
    while (cursor.next_capture(index)) {
        auto node = cursor.capture_node(index);
        auto name = cursor.capture_name(index);
        auto str = text.node_string(node);
        float x = 0, y = 0;
        DWRITE_HIT_TEST_METRICS metrics;
        textLayout->HitTestTextPosition(node.start_byte() / sizeof(wchar_t),
                false, &x, &y, &metrics);
        auto pf = D2D1::RectF(x, y, width - x, height - y);
        brush->SetColor(D2D1::ColorF(color_table[name]));
        brush->SetOpacity(255);
        target->DrawTextW(str.c_str(), str.length(), textFormat, pf, brush);

        brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        brush->SetOpacity(200);

    }
}

DWRITE_TEXT_RANGE Editor::getSelectionRange()
{
    UINT32 caretBegin = caretAnchor;
    UINT32 caretEnd = caretPosition;
    if (caretBegin > caretEnd)
        std::swap(caretBegin, caretEnd);

    UINT32 textLength = (UINT32)(text.length());

    if (caretBegin > textLength)
        caretBegin = textLength;

    if (caretEnd > textLength)
        caretEnd = textLength;

    return { caretBegin,caretEnd - caretBegin };
}
