import struct,sys
SRC=sys.argv[1] if len(sys.argv)>1 else '/home/claude/mod2/modules/Location.dll'
DST=sys.argv[2] if len(sys.argv)>2 else '/home/claude/dllpatch/Location_v2.dll'
d=bytearray(open(SRC,'rb').read())
BASE=0x10000000
pe=struct.unpack_from('<I',d,0x3c)[0]
nsec=struct.unpack_from('<H',d,pe+6)[0]; opt=struct.unpack_from('<H',d,pe+20)[0]
sec_off=pe+24+opt
for i in range(nsec):
    o=sec_off+40*i
    if d[o:o+8].rstrip(b'\0')==b'.text': tsec=o
vsize,rva,rsize,rptr=struct.unpack_from('<IIII',d,tsec+8)
assert rptr==rva and rsize>=vsize
def fo(va): return va-BASE-rva+rptr
CAVE=(BASE+rva+vsize+15)&~15
tail_end=BASE+rva+rsize
assert all(b==0 for b in d[fo(CAVE):fo(tail_end)]), 'tail is not empty (is this an already patched dll?)'

HOOK_JNE=0x1002037c; TRAMP=0x100203e7; DEFAULT=0x100203d8; STRICMP=0x1004de15; CORE_OPERAND=0x10018fe3
assert bytes(d[fo(HOOK_JNE):fo(HOOK_JNE)+2])==b'\x75\x5a'
assert bytes(d[fo(TRAMP):fo(TRAMP)+9])==b'\xcc'*9
assert bytes(d[fo(DEFAULT):fo(DEFAULT)+2])==b'\x5f\x5e'
assert bytes(d[fo(CORE_OPERAND)-2:fo(CORE_OPERAND)+4])==bytes.fromhex('8b0d24f50510')

code=bytearray(); fix=[]; labels={}
def emit(b): code.extend(b)
def rel_abs(t): fix.append((len(code),'rel',t)); emit(b'\0'*4)
def rel_lab(n): fix.append((len(code),'rellab',n)); emit(b'\0'*4)
def pic_lab(n): fix.append((len(code),'piclab',n)); emit(b'\0'*4)
def pic_abs(v): fix.append((len(code),'picabs',v)); emit(b'\0'*4)
def label(n): labels[n]=len(code)

EPI0 =bytes.fromhex('5f5e5d315c')  # placeholder (not used)
EPI_RET1=bytes.fromhex('5f5e5db8010000005b81c4f0010000c20400')   # pop edi,esi,ebp; mov eax,1; pop ebx; add esp,0x1f0; ret 4
EPI_EAX =bytes.fromhex('5f5e5d5b81c4f0010000c20400')            # same, eax is already set

def cmp_cmd(strlabel,nextlabel):
    emit(b'\x8D\x83'); pic_lab(strlabel)      # lea eax,[ebx+str]
    emit(b'\x50')                              # push eax
    emit(b'\x8D\x4C\x24\x20')                  # lea ecx,[esp+0x20]  (the command buffer was at esp+0x1c)
    emit(b'\x51')                              # push ecx
    emit(b'\xE8'); rel_abs(STRICMP)            # call stricmp
    emit(b'\x83\xC4\x08')                      # add esp,8
    emit(b'\x85\xC0')                          # test eax,eax
    emit(b'\x0F\x85')
    if nextlabel=='DEFAULT': rel_abs(DEFAULT)
    else: rel_lab(nextlabel)                   # jne next
def get_long():   emit(b'\x8B\x16\x8B\xCE\xFF\x52\x0C')     # mov edx,[esi]; mov ecx,esi; call [edx+0xc]  -> eax
def get_var():    emit(b'\x8B\x16\x8B\xCE\xFF\x52\x28')     # ... call [edx+0x28]  -> VDATA*
def j_fail(lab):  emit(b'\x0F\x84'); rel_lab(lab)           # je lab
def jae_fail(lab):emit(b'\x0F\x83'); rel_lab(lab)           # jae lab
def to_player(idx_reg_push, failslab):
    # core -> model -> animation -> Player(index)   (eax = player, 0 on failure => jump to failslab)
    emit(b'\x8B\x83'); pic_abs(CORE_OPERAND)   # mov eax,[ebx+disp]
    emit(b'\x8B\x08\x8B\x11')                  # mov ecx,[eax]; mov edx,[ecx]
    emit(b'\x8D\x87\xA8\x09\x00\x00')          # lea eax,[edi+0x9a8]
    emit(b'\x50\xFF\x52\x64')                  # push eax; call [edx+0x64]
    emit(b'\x85\xC0'); j_fail(failslab)
    emit(b'\x8B\x10\x8B\xC8\xFF\x52\x68')      # mov edx,[eax]; mov ecx,eax; call [edx+0x68]
    emit(b'\x85\xC0'); j_fail(failslab)
    emit(b'\x8B\x10\x55\x8B\xC8\xFF\x52\x04')  # mov edx,[eax]; push ebp; mov ecx,eax; call [edx+4]
    emit(b'\x85\xC0'); j_fail(failslab)

# ---------------- entry
emit(b'\xE8\x00\x00\x00\x00'); BASEPOS=len(code); emit(b'\x5B')      # call $+5 ; pop ebx
# ---------------- GetAnim(player, &name)
cmp_cmd('s_getanim','L_getstate')
get_long(); emit(b'\x8B\xE8')              # mov ebp,eax (index)
get_var();  emit(b'\x8B\xF0')              # mov esi,eax (VDATA*)
emit(b'\x83\xFD\x08'); jae_fail('A_fail')
to_player(None,'A_fail')
emit(b'\x8B\x10\x8B\xC8\xFF\x52\x04')      # GetAction
emit(b'\x85\xC0'); j_fail('A_fail')
emit(b'\x50\x8B\x16\x8B\xCE\xFF\x52\x2C')  # push eax; VDATA::Set(const char*)
emit(EPI_RET1)
label('A_fail')
emit(b'\x8D\x83'); pic_lab('s_empty'); emit(b'\x50\x8B\x16\x8B\xCE\xFF\x52\x2C')
emit(b'\xE9'); rel_abs(DEFAULT)
# ---------------- GetFightState() -> 0x10000 | state | sub<<8
label('L_getstate')
cmp_cmd('s_getstate','L_setstate')
emit(b'\x8B\x87\x4C\x08\x00\x00')          # mov eax,[edi+0x84c]
emit(b'\x25\xFF\x00\x00\x00')              # and eax,0xff
emit(b'\x8B\x8F\x58\x08\x00\x00')          # mov ecx,[edi+0x858]
emit(b'\x81\xE1\xFF\x00\x00\x00')          # and ecx,0xff
emit(b'\xC1\xE1\x08\x0B\xC1')              # shl ecx,8 ; or eax,ecx
emit(b'\x0D\x00\x00\x01\x00')              # or eax,0x10000
emit(EPI_EAX)
# ---------------- SetFightState(state, sub): the request of a state of the fight state machine
label('L_setstate')
cmp_cmd('s_setstate','L_animspeed')
get_long(); emit(b'\x8B\xE8')              # mov ebp,eax (state)
get_long()                                 # eax = sub
emit(b'\x89\xAF\x54\x08\x00\x00')          # mov [edi+0x854],ebp   (the next state)
emit(b'\x89\x87\x58\x08\x00\x00')          # mov [edi+0x858],eax   (its variant)
emit(b'\xC6\x87\x10\x01\x00\x00\x00')      # mov byte [edi+0x110],0
emit(EPI_RET1)
# ---------------- AnimSpeed(player, percent)
label('L_animspeed')
cmp_cmd('s_animspeed','DEFAULT')
get_long(); emit(b'\x8B\xE8')              # mov ebp,eax (player)
get_long(); emit(b'\x50')                  # push eax (percent)
emit(b'\x83\xFD\x08'); jae_fail('D_fail')
to_player(None,'D_fail')
emit(b'\x8B\xF0')                          # mov esi,eax (player)
emit(b'\xDB\x04\x24')                      # fild dword ptr [esp]
emit(b'\xD8\x8B'); pic_lab('c_001')        # fmul dword ptr [ebx+c_001]
emit(b'\xD9\x1C\x24')                      # fstp dword ptr [esp]
emit(b'\x8B\x16\x8B\xCE\xFF\x52\x24')      # mov edx,[esi]; mov ecx,esi; call [edx+0x24]  (SetSpeed(float): the float is on the stack)
emit(EPI_RET1)
label('D_fail')
emit(b'\x83\xC4\x04')                      # add esp,4
emit(b'\xE9'); rel_abs(DEFAULT)
# ---------------- data
emit(b'\xCC'*((4-len(code)%4)%4))
label('c_001'); emit(struct.pack('<f',0.01))
label('s_getanim'); emit(b'GetAnim\0')
label('s_getstate'); emit(b'GetFightState\0')
label('s_setstate'); emit(b'SetFightState\0')
label('s_animspeed'); emit(b'AnimSpeed\0')
label('s_empty'); emit(b'\0')
emit(b'\xCC'*((16-len(code)%16)%16))
assert CAVE+len(code)<=tail_end
for pos,kind,t in fix:
    if kind=='rel': v=t-(CAVE+pos+4)
    elif kind=='rellab': v=(CAVE+labels[t])-(CAVE+pos+4)
    elif kind=='piclab': v=(CAVE+labels[t])-(CAVE+BASEPOS)
    elif kind=='picabs': v=t-(CAVE+BASEPOS)
    code[pos:pos+4]=struct.pack('<i',v)
d[fo(CAVE):fo(CAVE)+len(code)]=code
d[fo(HOOK_JNE)+1]=TRAMP-(HOOK_JNE+2)
d[fo(TRAMP):fo(TRAMP)+5]=b'\xE9'+struct.pack('<i',CAVE-(TRAMP+5))
struct.pack_into('<I',d,tsec+8,max(vsize,(CAVE+len(code))-BASE-rva))
open(DST,'wb').write(d)
print('cave %#x, %d bytes, written %s'%(CAVE,len(code),DST))
