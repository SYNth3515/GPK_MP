import struct,sys,subprocess
SRC='/home/claude/mod2/modules/Location.dll'
DST='/home/claude/dllpatch/Location.dll'
d=bytearray(open(SRC,'rb').read())
BASE=0x10000000
pe=struct.unpack_from('<I',d,0x3c)[0]
nsec=struct.unpack_from('<H',d,pe+6)[0]; opt=struct.unpack_from('<H',d,pe+20)[0]
sec_off=pe+24+opt
# .text section header
for i in range(nsec):
    o=sec_off+40*i
    if d[o:o+8].rstrip(b'\0')==b'.text': tsec=o
vsize,rva,rsize,rptr=struct.unpack_from('<IIII',d,tsec+8)
print('.text vsize=%#x rva=%#x rsize=%#x rptr=%#x'%(vsize,rva,rsize,rptr))
assert rptr==rva and rsize>=vsize
def fo(va): return va-BASE-rva+rptr
tail_start=BASE+rva+vsize
CAVE=(tail_start+15)&~15
tail_end=BASE+rva+rsize
print('cave at %#x, free until %#x (%d bytes)'%(CAVE,tail_end,tail_end-CAVE))
assert all(b==0 for b in d[fo(CAVE):fo(tail_end)]), 'tail is not empty'

HOOK_JNE=0x1002037c         # 75 5a : jne default  (command is not "GunBelt")
TRAMP=0x100203e7            # int3 padding after the function epilogue
DEFAULT=0x100203d8          # return 0
STRICMP=0x1004de15
CORE_OPERAND=0x10018fe3     # operand of "mov ecx,ds:[core]" (the loader fixes it up): PIC access to the core pointer
assert bytes(d[fo(HOOK_JNE):fo(HOOK_JNE)+2])==b'\x75\x5a', d[fo(HOOK_JNE):fo(HOOK_JNE)+2]
assert bytes(d[fo(TRAMP):fo(TRAMP)+9])==b'\xcc'*9
assert bytes(d[fo(DEFAULT):fo(DEFAULT)+2])==b'\x5f\x5e'
assert bytes(d[fo(STRICMP):fo(STRICMP)+1])!=b''
assert bytes(d[fo(CORE_OPERAND)-2:fo(CORE_OPERAND)+4])==bytes.fromhex('8b0d')+bytes.fromhex('24f50510'), d[fo(CORE_OPERAND)-2:fo(CORE_OPERAND)+4].hex()

code=bytearray()
fix=[]   # (pos_in_code, kind, target)  kind: 'rel32' (jump/call to absolute VA) or 'lab' (label inside cave), 'pic' (disp from ebx)
labels={}
def emit(b): code.extend(b)
def rel_abs(target):    # rel32 to an absolute VA, resolved later
    fix.append((len(code),'rel',target)); emit(b'\0\0\0\0')
def rel_lab(name):
    fix.append((len(code),'rellab',name)); emit(b'\0\0\0\0')
def pic_lab(name):
    fix.append((len(code),'piclab',name)); emit(b'\0\0\0\0')
def pic_abs(va):
    fix.append((len(code),'picabs',va)); emit(b'\0\0\0\0')

# --- PIC base: ebx = address of the instruction after the call
emit(b'\xE8\x00\x00\x00\x00')          # call $+5
BASEPOS=len(code)
emit(b'\x5B')                           # pop ebx   (ebx = CAVE+5)
# --- is the command "GetAnim"?
emit(b'\x8D\x83'); pic_lab('s_cmd')     # lea eax,[ebx+s_cmd]
emit(b'\x50')                           # push eax
emit(b'\x8D\x4C\x24\x20')               # lea ecx,[esp+0x20]  (the command buffer was at esp+0x1c)
emit(b'\x51')                           # push ecx
emit(b'\xE8'); rel_abs(STRICMP)         # call stricmp
emit(b'\x83\xC4\x08')                   # add esp,8
emit(b'\x85\xC0')                       # test eax,eax
emit(b'\x0F\x85'); rel_abs(DEFAULT)     # jne default  (some other command)
# --- the arguments: long index, variable to write
emit(b'\x8B\x16\x8B\xCE\xFF\x52\x0C')   # mov edx,[esi]; mov ecx,esi; call [edx+0xc]   (Long)
emit(b'\x8B\xE8')                       # mov ebp,eax                    (index)
emit(b'\x8B\x16\x8B\xCE\xFF\x52\x28')   # mov edx,[esi]; mov ecx,esi; call [edx+0x28]  (VDATA* of the "e" argument)
emit(b'\x8B\xF0')                       # mov esi,eax                    (esi = VDATA*)
emit(b'\x83\xFD\x08')                   # cmp ebp,8
emit(b'\x0F\x83'); rel_lab('fail')      # jae fail
# --- core -> model of the character -> animation -> player -> action name
emit(b'\x8B\x83'); pic_abs(CORE_OPERAND)  # mov eax,[ebx+disp]   (eax = address of the core pointer)
emit(b'\x8B\x08')                       # mov ecx,[eax]          (core)
emit(b'\x8B\x11')                       # mov edx,[ecx]
emit(b'\x8D\x87\xA8\x09\x00\x00')       # lea eax,[edi+0x9a8]    (entity id of the model)
emit(b'\x50')                           # push eax
emit(b'\xFF\x52\x64')                   # call [edx+0x64]        (GetEntityPointer)
emit(b'\x85\xC0'); emit(b'\x0F\x84'); rel_lab('fail')
emit(b'\x8B\x10\x8B\xC8\xFF\x52\x68')   # mov edx,[eax]; mov ecx,eax; call [edx+0x68]   (GetAnimation)
emit(b'\x85\xC0'); emit(b'\x0F\x84'); rel_lab('fail')
emit(b'\x8B\x10')                       # mov edx,[eax]
emit(b'\x55')                           # push ebp               (player index)
emit(b'\x8B\xC8\xFF\x52\x04')           # mov ecx,eax; call [edx+4]   (Player(index))
emit(b'\x85\xC0'); emit(b'\x0F\x84'); rel_lab('fail')
emit(b'\x8B\x10\x8B\xC8\xFF\x52\x04')   # mov edx,[eax]; mov ecx,eax; call [edx+4]   (GetAction)
emit(b'\x85\xC0'); emit(b'\x0F\x84'); rel_lab('fail')
emit(b'\x50')                           # push eax               (const char* name)
emit(b'\x8B\x16\x8B\xCE\xFF\x52\x2C')   # mov edx,[esi]; mov ecx,esi; call [edx+0x2c]   (VDATA::Set string)
emit(bytes.fromhex('5f5e5db8010000005b81c4f0010000c20400'))   # return 1 (the epilogue of the dispatcher)
labels['fail']=len(code)
emit(b'\x8D\x83'); pic_lab('s_empty')   # lea eax,[ebx+s_empty]
emit(b'\x50')
emit(b'\x8B\x16\x8B\xCE\xFF\x52\x2C')   # the variable gets ""
emit(b'\xE9'); rel_abs(DEFAULT)         # jmp default (returns 0)
labels['s_cmd']=len(code); emit(b'GetAnim\0')
labels['s_empty']=len(code); emit(b'\0')
emit(b'\xCC'*((16-len(code)%16)%16))
assert CAVE+len(code)<=tail_end
# --- resolve
for pos,kind,t in fix:
    if kind=='rel':
        v=t-(CAVE+pos+4)
    elif kind=='rellab':
        v=(CAVE+labels[t])-(CAVE+pos+4)
    elif kind=='piclab':
        v=(CAVE+labels[t])-(CAVE+BASEPOS)
    elif kind=='picabs':
        v=t-(CAVE+BASEPOS)
    code[pos:pos+4]=struct.pack('<i',v)
print('cave code %d bytes'%len(code))
d[fo(CAVE):fo(CAVE)+len(code)]=code
# --- the hook: "jne default" -> "jne trampoline", the trampoline jumps to the cave
d[fo(HOOK_JNE)+1]=TRAMP-(HOOK_JNE+2)
assert TRAMP-(HOOK_JNE+2)==0x69
d[fo(TRAMP):fo(TRAMP)+5]=b'\xE9'+struct.pack('<i',CAVE-(TRAMP+5))
# --- the loader must map the whole raw data of .text (the cave is in its tail)
struct.pack_into('<I',d,tsec+8,max(vsize,(CAVE+len(code))-BASE-rva))
open(DST,'wb').write(d)
print('written',DST)
