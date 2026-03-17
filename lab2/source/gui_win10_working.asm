format PE GUI 4.0
entry start

include 'win32a.inc'



ID_EDIT_INPUT      = 1001
ID_EDIT_OUTPUT     = 1002
ID_EDIT_OUTPUT2    = 1006
ID_EDIT_SEED       = 1007
ID_EDIT_FILE       = 1003
ID_BUTTON_PROCESS  = 1004
ID_BUTTON_CLEAR    = 1005
ID_EDIT_SRC_BITS   = 1008
ID_EDIT_DST_BITS   = 1009


WIN_MIN_W = 600
WIN_MIN_H = 650
WIN_MAX_W = 1400
WIN_MAX_H = 1100
WIN_INIT_W = 760
WIN_INIT_H = 780


hwnd     = 8
msg_code = 12
wparam   = 16
lparam   = 20


section '.data' data readable writeable


    window_title    db 'LFSR',0
    button_process  db 'Зашифровать',0
    button_clear    db 'Расшифровать',0
    

    label_input     db 'Начальное состояние регистра (биты 0/1):',0
    label_output    db 'Ключ LFSR - первые 150 бит:',0
    label_output2   db 'Имя выходного файла:',0
    label_seed      db 'Seed (hex):',0
    label_file      db 'Путь к входному файлу:',0
    label_src_bits  db 'Входной файл - первые 150 бит:',0
    label_dst_bits  db 'Выходной файл - первые 150 бит:',0


    input_buffer    db 256 dup (0)
    output_buffer   db 256 dup (0)
    seed_buffer     db 32 dup (0)
    file_buffer     db 512 dup (0)
    src_bits_buffer db 256 dup (0)
    dst_bits_buffer db 256 dup (0)


    msg_too_many_bits db 'Введено больше 25 бит. Лишние символы будут обрезаны.',0
    msg_invalid_chars db 'В строке ввода есть символы кроме 0 и 1. Лишние символы будут обрезаны.',0
    msg_bits_padded db 'Введено меньше 25 бит. Недостающие будут добавлены нулями.',0
    msg_zero_reg    db 'Регистр из всех нулей недопустим для LFSR (застрянет в нуле навсегда). Установлен в 1.',0
    msg_file_empty db 'Поле файла пустое.',0
    msg_file_not_found db 'Файл не найден.',0
    msg_cleared db 'Поле ввода пусто. Обработка не выполнена.',0
    msg_file_open_error db 'Ошибка открытия файла.',0
    msg_file_name_long db 'Имя выходного файла слишком длинное.',0

    bit_index db 0
    file_buffer_enc db 512 dup(0)
    file_byte_buffer db 1 dup(0)
    src_preview_buffer db 19 dup(0)
    dst_preview_buffer db 19 dup(0)
    lfsr_reg dd 0
    lfsr_bit_count dd 0
    src_preview_count dd 0
    preview_bits_to_show dd 150
    read_size dd 0
    write_size dd 0


    suffix_enc      db '_enc',0
    suffix_dec      db '_dec',0
    process_suffix  dd suffix_enc


    msg_error       db 'Ошибка',0
    

    window_class    db 'FASM_GUI_Window',0
    edit_class      db 'EDIT',0
    button_class    db 'BUTTON',0
    static_class    db 'STATIC',0
    

    hEditInput      dd 0
    hEditOutput     dd 0
    hEditOutput2    dd 0
    hEditSeed       dd 0
    hEditFile       dd 0
    hButtonProcess  dd 0
    hButtonClear    dd 0
    hEditSrcBits    dd 0
    hEditDstBits    dd 0
    hMainWnd        dd 0
    hLabelInput     dd 0
    hLabelFile      dd 0
    hLabelSeed      dd 0
    hLabelOutput2   dd 0
    hLabelOutput    dd 0
    hLabelSrcBits   dd 0
    hLabelDstBits   dd 0
    

    msg_struct MSG
    empty_string    db 0
    wc WNDCLASS ?




section '.code' code readable executable

start:

    invoke GetModuleHandle, 0
    mov [wc.hInstance], eax
    

    mov [wc.style], 0
    mov [wc.lpfnWndProc], window_proc
    mov [wc.cbClsExtra], 0
    mov [wc.cbWndExtra], 0
    mov eax, [wc.hInstance]
    mov [wc.hInstance], eax
    invoke LoadIcon, 0, IDI_APPLICATION
    mov [wc.hIcon], eax
    invoke LoadCursor, 0, IDC_ARROW
    mov [wc.hCursor], eax
    mov [wc.hbrBackground], COLOR_BTNFACE+1
    mov [wc.lpszMenuName], 0
    mov [wc.lpszClassName], window_class
    

    invoke RegisterClass, wc
    test eax, eax
    jz .error_exit
    

    invoke CreateWindowEx, 0, window_class, window_title, WS_VISIBLE+WS_CAPTION+WS_SYSMENU+WS_MINIMIZEBOX, 100, 50, WIN_INIT_W, WIN_INIT_H, 0, 0, [wc.hInstance], 0
    test eax, eax
    jz .error_exit
    mov [hMainWnd], eax


    invoke ShowWindow, eax, SW_SHOWDEFAULT
    invoke UpdateWindow, eax


.message_loop:
    invoke GetMessage, msg_struct, 0, 0, 0
    cmp eax, 0
    je .end_loop
    
    invoke TranslateMessage, msg_struct
    invoke DispatchMessage, msg_struct
    jmp .message_loop

.error_exit:
    invoke MessageBox, 0, msg_error, window_title, MB_ICONERROR+MB_OK
    
.end_loop:
    invoke ExitProcess, [msg_struct.wParam]




window_proc:
    push ebp
    mov ebp, esp
    

    mov eax, [ebp+msg_code]
    
    cmp eax, WM_DESTROY
    je .wm_destroy
    
    cmp eax, WM_CREATE
    je .wm_create
    
    cmp eax, WM_COMMAND
    je .wm_command

    cmp eax, WM_GETMINMAXINFO
    je .wm_getminmaxinfo


    invoke DefWindowProc, [ebp+hwnd], [ebp+msg_code], [ebp+wparam], [ebp+lparam]
    pop ebp
    ret 16



.wm_create:
    push ebx


    invoke CreateWindowEx, 0, static_class, label_input, WS_VISIBLE+WS_CHILD, 10, 10, 720, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelInput], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_AUTOHSCROLL, 10, 30, 720, 24, [ebp+hwnd], ID_EDIT_INPUT, [wc.hInstance], 0
    mov [hEditInput], eax


    invoke CreateWindowEx, 0, static_class, label_file, WS_VISIBLE+WS_CHILD, 10, 60, 720, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelFile], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_AUTOHSCROLL, 10, 80, 720, 24, [ebp+hwnd], ID_EDIT_FILE, [wc.hInstance], 0
    mov [hEditFile], eax


    invoke CreateWindowEx, 0, static_class, label_seed, WS_VISIBLE+WS_CHILD, 10, 110, 200, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelSeed], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_AUTOHSCROLL+ES_READONLY, 10, 130, 200, 24, [ebp+hwnd], ID_EDIT_SEED, [wc.hInstance], 0
    mov [hEditSeed], eax


    invoke CreateWindowEx, 0, static_class, label_output2, WS_VISIBLE+WS_CHILD, 220, 110, 520, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelOutput2], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_AUTOHSCROLL+ES_READONLY, 220, 130, 510, 24, [ebp+hwnd], ID_EDIT_OUTPUT2, [wc.hInstance], 0
    mov [hEditOutput2], eax


    invoke CreateWindowEx, 0, button_class, button_process, WS_VISIBLE+WS_CHILD+BS_PUSHBUTTON, 10, 166, 170, 32, [ebp+hwnd], ID_BUTTON_PROCESS, [wc.hInstance], 0
    mov [hButtonProcess], eax
    invoke CreateWindowEx, 0, button_class, button_clear, WS_VISIBLE+WS_CHILD+BS_PUSHBUTTON, 190, 166, 170, 32, [ebp+hwnd], ID_BUTTON_CLEAR, [wc.hInstance], 0
    mov [hButtonClear], eax


    invoke CreateWindowEx, 0, static_class, label_output, WS_VISIBLE+WS_CHILD, 10, 214, 720, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelOutput], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_MULTILINE+ES_AUTOVSCROLL+ES_READONLY+WS_VSCROLL, 10, 236, 720, 90, [ebp+hwnd], ID_EDIT_OUTPUT, [wc.hInstance], 0
    mov [hEditOutput], eax


    invoke CreateWindowEx, 0, static_class, label_src_bits, WS_VISIBLE+WS_CHILD, 10, 334, 720, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelSrcBits], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_MULTILINE+ES_AUTOVSCROLL+ES_READONLY+WS_VSCROLL, 10, 356, 720, 90, [ebp+hwnd], ID_EDIT_SRC_BITS, [wc.hInstance], 0
    mov [hEditSrcBits], eax


    invoke CreateWindowEx, 0, static_class, label_dst_bits, WS_VISIBLE+WS_CHILD, 10, 454, 720, 18, [ebp+hwnd], 0, [wc.hInstance], 0
    mov [hLabelDstBits], eax
    invoke CreateWindowEx, WS_EX_CLIENTEDGE, edit_class, 0, WS_VISIBLE+WS_CHILD+WS_BORDER+ES_LEFT+ES_MULTILINE+ES_AUTOVSCROLL+ES_READONLY+WS_VSCROLL, 10, 476, 720, 90, [ebp+hwnd], ID_EDIT_DST_BITS, [wc.hInstance], 0
    mov [hEditDstBits], eax

    pop ebx
    xor eax, eax
    pop ebp
    ret 16




.wm_command:
    push ebx
    push esi
    push edi
    

    mov eax, [ebp+wparam]
    and eax, 0xFFFF
    
    cmp eax, ID_BUTTON_PROCESS
    je .process_button
    
    cmp eax, ID_BUTTON_CLEAR
    je .clear_button
    
    jmp .wm_command_done




.process_button:

    mov dword [process_suffix], suffix_enc
    call process_lfsr
    jmp .wm_command_done





.clear_button:

    mov dword [process_suffix], suffix_dec
    call process_lfsr
    jmp .wm_command_done

.wm_command_done:
    pop edi
    pop esi
    pop ebx
    xor eax, eax
    pop ebp
    ret 16





.wm_size:

    xor eax, eax
    pop ebp
    ret 16




.wm_getminmaxinfo:
    mov eax, [ebp+lparam]

    mov dword [eax+24], WIN_MIN_W
    mov dword [eax+28], WIN_MIN_H

    mov dword [eax+32], WIN_MAX_W
    mov dword [eax+36], WIN_MAX_H
    xor eax, eax
    pop ebp
    ret 16




.wm_destroy:
    invoke PostQuitMessage, 0
    xor eax, eax
    pop ebp
    ret 16




include 'gui_win10_logic.inc'



section '.idata' import data readable writeable

    library kernel32, 'kernel32.dll', user32, 'user32.dll'

    import kernel32, \
        ExitProcess, 'ExitProcess', \
        GetModuleHandle, 'GetModuleHandleA', \
        GetFileAttributes, 'GetFileAttributesA', \
        CreateFile, 'CreateFileA', \
        ReadFile, 'ReadFile', \
        WriteFile, 'WriteFile', \
        CloseHandle, 'CloseHandle', \
        SetFilePointer, 'SetFilePointer'
        
    import user32, \
        RegisterClass, 'RegisterClassA', \
        CreateWindowEx, 'CreateWindowExA', \
        DefWindowProc, 'DefWindowProcA', \
        GetMessage, 'GetMessageA', \
        TranslateMessage, 'TranslateMessage', \
        DispatchMessage, 'DispatchMessageA', \
        LoadIcon, 'LoadIconA', \
        LoadCursor, 'LoadCursorA', \
        MessageBox, 'MessageBoxA', \
        PostQuitMessage, 'PostQuitMessage', \
        SendMessage, 'SendMessageA', \
        ShowWindow, 'ShowWindow', \
        UpdateWindow, 'UpdateWindow', \
        MoveWindow, 'MoveWindow'