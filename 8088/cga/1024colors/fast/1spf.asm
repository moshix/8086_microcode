org 0
cpu 8086

  cli

  ; Copy data
  mov ax,cs
  mov ds,ax
  mov ax,0xb800
  mov es,ax
;  mov cx,8000
;  mov si,data
  xor di,di
  cld
;  rep movsw
%rep 178
  mov ax,0x0f0a
  stosw
  mov ax,0
  mov cx,45
  rep stosw
%endrep

%macro waitForDisplayEnable 0
  %%waitForDisplayEnable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz %%waitForDisplayEnable     ; 2 0 2
%endmacro

%macro waitForDisplayDisable 0
  %%waitForDisplayDisable
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jz %%waitForDisplayDisable     ; 2 0 2
%endmacro

%macro waitForVerticalSync 0
  %%waitForVerticalSync
    in al,dx
    test al,8
    jz %%waitForVerticalSync
%endmacro

%macro waitForNoVerticalSync 0
  %%waitForNoVerticalSync
    in al,dx
    test al,8
    jnz %%waitForNoVerticalSync
%endmacro

  ; Mode                                                09
  ;      1 +HRES                                         1
  ;      2 +GRPH                                         0
  ;      4 +BW                                           0
  ;      8 +VIDEO ENABLE                                 8
  ;   0x10 +1BPP                                         0
  ;   0x20 +ENABLE BLINK                                 0
  mov dx,0x3d8
  mov al,0x09
  out dx,al

  ; Palette                                             00
  ;      1 +OVERSCAN B                                   0
  ;      2 +OVERSCAN G                                   2
  ;      4 +OVERSCAN R                                   4
  ;      8 +OVERSCAN I                                   0
  ;   0x10 +BACKGROUND I                                 0
  ;   0x20 +COLOR SEL                                    0
  inc dx
  mov al,0
  out dx,al

  mov dl,0xd4

  ;   0xff Horizontal Total                             71
  mov ax,0x7100
  out dx,ax

  ;   0xff Horizontal Displayed                         50
  mov ax,0x5001
  out dx,ax

  ;   0xff Horizontal Sync Position                     5a
  mov ax,0x5a02
  out dx,ax

  ;   0x0f Horizontal Sync Width                        0d
  mov ax,0x0003
  out dx,ax

  ;   0x7f Vertical Total                               3d
  mov ax,0x0004
  out dx,ax

  ;   0x1f Vertical Total Adjust                        00
  mov ax,0x0005
  out dx,ax

  ;   0x7f Vertical Displayed                           02
  mov ax,0x0106
  out dx,ax

  ;   0x7f Vertical Sync Position                       18
  mov ax,0x3807
  out dx,ax

  ;   0x03 Interlace Mode                               02   0 = non interlaced, 1 = interlace sync, 3 = interlace sync and video
  mov ax,0x0308
  out dx,ax

  ;   0x1f Max Scan Line Address                        00
  mov ax,0x0009
  out dx,ax

  ; Cursor Start                                        06
  ;   0x1f Cursor Start                                  6
  ;   0x60 Cursor Mode                                   0
  mov ax,0x060a
  out dx,ax

  ;   0x1f Cursor End                                   07
  mov ax,0x080b
  out dx,ax

  ;   0x3f Start Address (H)                            00
  mov ax,0x000c
  out dx,ax

  ;   0xff Start Address (L)                            00
  mov ax,0x000d
  out dx,ax

  ;   0x3f Cursor (H)                                   03
  mov ax,0x030e
  out dx,ax

  ;   0xff Cursor (L)                                   c0
  mov ax,0xc00f
  out dx,ax

  mov dl,0xda
;  waitForVerticalSync

  xor bx,bx
  mov cx,65535

;  waitForDisplayEnable
loopTop:
  waitForDisplayEnable
  mov dl,0xd4
  mov ah,bh
  mov al,0x0c
  out dx,ax
  mov ah,bl
  inc ax
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable
  inc bx
  loop loopTop

  int 0x60
  mov dl,0xd9
  mov al,0x0f
  out dx,al

  xor bx,bx
  mov cx,65535
  mov dl,0xda

;  waitForDisplayEnable
loopTop1:
  waitForDisplayEnable
  mov dl,0xd9
  mov al,bl
  out dx,al
  mov dl,0xd4
  mov ah,bh
  mov al,0x0c
  out dx,ax
  mov ah,bl
  inc ax
  out dx,ax
  mov dl,0xda
  waitForDisplayDisable
  inc bx
  loop loopTop1

  int 0x67

data:

