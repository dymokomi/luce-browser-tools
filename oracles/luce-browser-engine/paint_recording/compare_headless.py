import struct,zlib,subprocess,glob,os,sys,time
ORA='/Users/sedov/Dev/luce_dev/luce-browser-tools/oracles/luce-browser-engine/paint_recording'
B='/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Build/release/bin/Ladybird.app/Contents/MacOS/Ladybird'
def unesc(s):
    out=[];i=0
    while i<len(s):
        if s[i]=='\\' and i+1<len(s):
            c=s[i+1]; out.append('\n' if c=='n' else '\t' if c=='t' else c); i+=2
        else: out.append(s[i]); i+=1
    return ''.join(out)
def png(path):
    d=open(path,'rb').read(); w,h=struct.unpack('>II',d[16:24]); i=8; idat=b''
    while i<len(d):
        n=struct.unpack('>I',d[i:i+4])[0]; t=d[i+4:i+8]
        if t==b'IDAT': idat+=d[i+8:i+8+n]
        i+=12+n
    raw=zlib.decompress(idat); bpp=4; stride=w*4; rows=[]; prev=bytearray(stride); p=0
    for y in range(h):
        f=raw[p]; p+=1; line=bytearray(raw[p:p+stride]); p+=stride
        for x in range(stride):
            a=line[x-bpp] if x>=bpp else 0; b=prev[x]; c=prev[x-bpp] if x>=bpp else 0
            if f==1: line[x]=(line[x]+a)&255
            elif f==2: line[x]=(line[x]+b)&255
            elif f==3: line[x]=(line[x]+(a+b)//2)&255
            elif f==4:
                pp=a+b-c; pa=abs(pp-a); pb=abs(pp-b); pc=abs(pp-c)
                pr=a if pa<=pb and pa<=pc else (b if pb<=pc else c); line[x]=(line[x]+pr)&255
        rows.append(line); prev=line
    return w,h,rows
exp=open(ORA+'/expected.txt').read().split('\n'); cases=[]
cur=None
for l in exp:
    if l.startswith('case '):
        cur=[l[5:],[]]; cases.append(cur)
    elif cur: cur[1].append(l)
for head,body in cases:
    if not head.startswith('px '): continue
    mode,html,css=head.split('\t'); _,x,y,w,h=mode.split(); w=int(w); h=int(h)
    words=[]
    for l in body:
        l=l.strip()
        if l.startswith('outside'): break
        for tok in l.split():
            n,v=tok.split('x'); words+= [int(v,16)]*int(n)
    doc=unesc(html).replace('<!DOCTYPE html>','<!DOCTYPE html><style>'+unesc(css)+'</style>')
    open('doc.html','w').write(doc)
    for f in glob.glob(os.path.expanduser('~/Downloads/screenshot-*.png')): pass
    before=set(glob.glob(os.path.expanduser('~/Downloads/screenshot-*.png')))
    subprocess.run([B,'--headless=screenshot','--test-mode','--force-cpu-painting','--force-fontconfig','--screenshot-delay','1','file://'+os.getcwd()+'/doc.html'],capture_output=True)
    new=list(set(glob.glob(os.path.expanduser('~/Downloads/screenshot-*.png')))-before)
    shot=os.getcwd()+'/shot_%d.png'%len(glob.glob('shot_*.png')); os.rename(new[0],shot)
    W,H,rows=png(shot)
    diff=0;maxd=0
    for yy in range(h):
        for xx in range(w):
            r,g,b,a=rows[yy][xx*4:xx*4+4]; v=words[yy*w+xx]
            er,eg,eb,ea=(v>>16)&255,(v>>8)&255,v&255,v>>24
            d=max(abs(r-er),abs(g-eg),abs(b-eb),abs(a-ea))
            if d: diff+=1; maxd=max(maxd,d)
    print(shot, w,h,'differing',diff,'max',maxd)
