GENERIC_READ        equ 0x80000000
GENERIC_WRITE       equ 0x40000000
OPEN_EXISTING       equ 3
CREATE_ALWAYS       equ 2
FILE_ATTRIBUTE_NORMAL equ 0x80
FILE_BEGIN          equ 0


LFSR_KEY_OUTPUT_LEN equ 150
BITS_PREVIEW_BYTES  equ 19



check_file_field:

    push ecx
    push edx
    push esi


    invoke SendMessage, [hEditFile], WM_GETTEXT, 512, file_buffer
    cmp byte [file_buffer], 0
    je file_empty


    mov ecx, 0
    mov esi, file_buffer
check_path_loop:
    mov al, [esi]
    cmp al, 0
    je check_path_end
    inc ecx
    inc esi
    jmp check_path_loop
check_path_end:
    cmp ecx, 3
    jb file_empty


    invoke GetFileAttributes, file_buffer
    cmp eax, -1
    je file_not_found


    xor eax, eax
    jmp cf_exit

file_empty:

    invoke MessageBox, 0, msg_file_empty, window_title, MB_ICONERROR+MB_OK
    mov eax, 1
    jmp cf_exit

file_not_found:

    invoke MessageBox, 0, msg_file_not_found, window_title, MB_ICONERROR+MB_OK
    mov eax, 1
    jmp cf_exit

cf_exit:

    pop esi
    pop edx
    pop ecx
    ret

process_lfsr:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi


    call check_file_field
    cmp eax, 1
    je process_done


    invoke SendMessage, [hEditInput], WM_GETTEXT, 256, input_buffer
    cmp byte [input_buffer], 0
    je no_input


    mov esi, input_buffer
    xor ecx, ecx
    xor eax, eax
    xor edx, edx
    xor edi, edi

filter_loop_lfsr:
    mov bl, [esi]
    cmp bl, 0
    je filter_end_lfsr
    cmp bl, '0'
    je add_bit_zero_lfsr
    cmp bl, '1'
    je add_bit_one_lfsr


    mov edx, 1
    jmp skip_bit_lfsr

add_bit_zero_lfsr:
    cmp ecx, 25
    jae set_too_many_bits
    shl eax, 1
    inc ecx
    jmp skip_bit_lfsr

add_bit_one_lfsr:
    cmp ecx, 25
    jae set_too_many_bits
    shl eax, 1
    or eax, 1
    inc ecx
    jmp skip_bit_lfsr

set_too_many_bits:
    mov edi, 1
skip_bit_lfsr:
    inc esi
    jmp filter_loop_lfsr

filter_end_lfsr:

    cmp edx, 0
    je check_bit_count_lfsr
    push eax
    invoke MessageBox, 0, msg_invalid_chars, window_title, MB_ICONWARNING+MB_OK
    pop eax

check_bit_count_lfsr:

    cmp edi, 0
    jne too_many_bits_notify_lfsr


    cmp ecx, 25
    je reg_ready_lfsr
    push eax
    invoke MessageBox, 0, msg_bits_padded, window_title, MB_ICONWARNING+MB_OK
    pop eax
    jmp reg_ready_lfsr

too_many_bits_notify_lfsr:

    cmp edi, 0
    je reg_ready_lfsr
    push eax
    invoke MessageBox, 0, msg_too_many_bits, window_title, MB_ICONWARNING+MB_OK
    pop eax
    jmp reg_ready_lfsr

reg_ready_lfsr:

    and eax, 0x1FFFFFF




    cmp eax, 0
    jne init_ok_lfsr
    push eax
    invoke MessageBox, 0, msg_zero_reg, window_title, MB_ICONWARNING+MB_OK
    pop eax
    mov eax, 1

init_ok_lfsr:
    mov ebx, eax
    mov [lfsr_reg], ebx
    mov dword [preview_bits_to_show], LFSR_KEY_OUTPUT_LEN



    mov edi, seed_buffer
    mov byte [edi], '0'
    mov byte [edi+1], 'x'
    lea esi, [edi+2]

    mov edx, ebx
    shl edx, 4
    mov ecx, 7
seed_hex_loop:
    mov eax, edx
    shr eax, 28
    cmp al, 10
    jl seed_digit_dec
    add al, 'A' - 10
    jmp seed_store
seed_digit_dec:
    add al, '0'
seed_store:
    mov [esi], al
    inc esi
    shl edx, 4
    dec ecx
    jne seed_hex_loop

    mov byte [esi], 0
    invoke SendMessage, [hEditSeed], WM_SETTEXT, 0, seed_buffer



    mov esi, file_buffer
    mov edi, file_buffer_enc
    xor ecx, ecx
copy_path_loop:
    mov al, [esi]
    mov [edi], al
    inc esi
    inc edi
    inc ecx
    cmp ecx, 511
    jae file_name_too_long
    cmp al, 0
    jne copy_path_loop


    mov esi, file_buffer_enc
    mov ebx, esi
find_last_sep:
    mov al, [esi]
    cmp al, 0
    je find_ext_start
    cmp al, 0x5C
    je sep_found
    cmp al, '/'
    je sep_found
    inc esi
    jmp find_last_sep
sep_found:
    inc esi
    mov ebx, esi
    jmp find_last_sep

find_ext_start:

    mov esi, ebx
    xor ecx, ecx
find_dot_loop:
    mov al, [esi]
    cmp al, 0
    je dot_search_done
    cmp al, '.'
    jne next_dot
    mov ecx, esi
next_dot:
    inc esi
    jmp find_dot_loop

dot_search_done:

    mov edi, esi

    cmp ecx, 0
    jne dot_found

    mov esi, edi
    jmp insertion_point_ready

dot_found:
    mov esi, ecx

insertion_point_ready:



    mov edx, esi

insert_suffix:
    mov esi, edx


    mov eax, edi
    sub eax, file_buffer_enc
    add eax, 4
    cmp eax, 511
    ja file_name_too_long


    mov ebx, esi
    mov ecx, edi
shift_move:
    mov al, [ecx]
    mov [ecx+4], al
    dec ecx
    cmp ecx, ebx
    jae shift_move


    mov esi, [process_suffix]
    mov eax, [esi]
    mov [ebx], eax

suffix_done:

    invoke SendMessage, [hEditOutput2], WM_SETTEXT, 0, file_buffer_enc
    jmp file_name_ready

file_name_too_long:
    invoke MessageBox, 0, msg_file_name_long, window_title, MB_ICONERROR+MB_OK
    jmp process_done

file_name_ready:


    invoke CreateFile, file_buffer, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0
    mov esi, eax
    cmp esi, -1
    je file_open_error


    invoke ReadFile, esi, src_preview_buffer, BITS_PREVIEW_BYTES, read_size, 0
    mov eax, [read_size]
    mov [src_preview_count], eax



    shl eax, 3
    cmp eax, LFSR_KEY_OUTPUT_LEN
    jbe preview_bits_ready
    mov eax, LFSR_KEY_OUTPUT_LEN
preview_bits_ready:
    mov [preview_bits_to_show], eax


    call render_key_output


    invoke SetFilePointer, esi, 0, 0, FILE_BEGIN


    invoke CreateFile, file_buffer_enc, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0
    mov edi, eax
    cmp edi, -1
    je file_dst_open_error


    mov [lfsr_bit_count], 0
    mov [read_size], 0
    mov [write_size], 0

file_encrypt_loop:

    invoke ReadFile, esi, file_byte_buffer, 1, read_size, 0
    cmp [read_size], 1
    jne file_encrypt_end


    push esi
    call generate_key_byte
    pop esi

    mov al, [file_byte_buffer]
    xor al, dl
    mov [file_byte_buffer], al

    invoke WriteFile, edi, file_byte_buffer, 1, write_size, 0
    cmp [write_size], 1
    jne file_encrypt_end
    inc [lfsr_bit_count]
    jmp file_encrypt_loop
file_encrypt_end:

    invoke CloseHandle, esi
    invoke CloseHandle, edi


    call display_src_bits


    invoke CreateFile, file_buffer_enc, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0
    cmp eax, -1
    je process_done
    mov esi, eax
    invoke ReadFile, esi, dst_preview_buffer, BITS_PREVIEW_BYTES, read_size, 0
    invoke CloseHandle, esi
    push [read_size]
    call display_dst_bits

    jmp process_done

file_dst_open_error:
    invoke CloseHandle, esi
    jmp file_open_error

file_open_error:
    invoke MessageBox, 0, msg_file_open_error, window_title, MB_ICONERROR+MB_OK
    jmp proc_exit

no_input:

    invoke SendMessage, [hEditOutput], WM_SETTEXT, 0, msg_cleared

process_done:
    xor eax, eax
proc_exit:
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret







render_key_output:
    push eax
    push edx
    push esi
    push edi

    mov edx, [lfsr_reg]
    mov edi, output_buffer
    xor esi, esi

rk_loop:
    cmp esi, [preview_bits_to_show]
    jae rk_done

    mov eax, edx
    call lfsr_step
    mov edx, eax

    add bl, '0'
    mov [edi], bl
    inc edi
    inc esi
    jmp rk_loop

rk_done:
    mov byte [edi], 0
    invoke SendMessage, [hEditOutput], WM_SETTEXT, 0, output_buffer

    pop edi
    pop esi
    pop edx
    pop eax
    ret







display_src_bits:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi

    mov ecx, [src_preview_count]

    mov esi, src_preview_buffer
    mov edi, src_bits_buffer
    xor edx, edx

dsb_byte_loop:
    cmp ecx, 0
    je dsb_finish
    cmp edx, [preview_bits_to_show]
    jae dsb_finish

    mov bl, [esi]
    inc esi


    push ecx
    mov ecx, 8
dsb_bit_loop:
    cmp edx, [preview_bits_to_show]
    jae dsb_bit_end

    mov al, bl
    shr al, 7
    add al, '0'
    mov [edi], al
    inc edi
    inc edx

    shl bl, 1
    dec ecx
    jne dsb_bit_loop
dsb_bit_end:
    pop ecx
    dec ecx
    jmp dsb_byte_loop

dsb_finish:

    mov byte [edi], 0
    invoke SendMessage, [hEditSrcBits], WM_SETTEXT, 0, src_bits_buffer
dsb_done:
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret






display_dst_bits:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi

    mov ecx, [ebp+8]

    mov esi, dst_preview_buffer
    mov edi, dst_bits_buffer
    xor edx, edx

ddb_byte_loop:
    cmp ecx, 0
    je ddb_finish
    cmp edx, [preview_bits_to_show]
    jae ddb_finish

    mov bl, [esi]
    inc esi

    push ecx
    mov ecx, 8
ddb_bit_loop:
    cmp edx, [preview_bits_to_show]
    jae ddb_bit_end

    mov al, bl
    shr al, 7
    add al, '0'
    mov [edi], al
    inc edi
    inc edx

    shl bl, 1
    dec ecx
    jne ddb_bit_loop
ddb_bit_end:
    pop ecx
    dec ecx
    jmp ddb_byte_loop

ddb_finish:

    mov byte [edi], 0
    invoke SendMessage, [hEditDstBits], WM_SETTEXT, 0, dst_bits_buffer
ddb_done:
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret 4




lfsr_step:
    mov edx, eax
    shr edx, 24
    and edx, 1
    mov bl, dl
    mov ecx, eax
    shr ecx, 2
    and ecx, 1
    xor edx, ecx

    shl eax, 1
    or eax, edx
    and eax, 0x1FFFFFF
    ret

    
generate_key_byte:
    xor dl, dl
    mov esi, 8

gen_key_byte_loop:

    mov eax, [lfsr_reg]



    push edx
    call lfsr_step
    pop edx


    shl dl, 1
    or dl, bl

    mov [lfsr_reg], eax

    dec esi
    jne gen_key_byte_loop
    ret
