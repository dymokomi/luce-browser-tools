import struct,sys
d=open(sys.argv[1],'rb').read()
n=struct.unpack('>H',d[4:6])[0]
tables={}
for i in range(n):
    tag,cs,off,ln=struct.unpack('>4sIII',d[12+16*i:28+16*i]); tables[tag]=(off,ln)
for T in [b'GSUB',b'GPOS']:
    if T not in tables: continue
    base=tables[T][0]
    sl,fl,ll=struct.unpack('>HHH',d[base+4:base+10])
    sc=struct.unpack('>H',d[base+sl:base+sl+2])[0]
    print(T)
    for i in range(sc):
        tag,off=struct.unpack('>4sH',d[base+sl+2+6*i:base+sl+8+6*i])
        s=base+sl+off
        dl,lc=struct.unpack('>HH',d[s:s+4])
        feats=[]
        if dl:
            L=s+dl
            req,fc=struct.unpack('>HH',d[L+2:L+6])
            fis=struct.unpack('>'+'H'*fc,d[L+6:L+6+2*fc])
            for fi in fis:
                ftag,foff=struct.unpack('>4sH',d[base+fl+2+6*fi:base+fl+8+6*fi])
                F=base+fl+foff
                lcnt=struct.unpack('>H',d[F+2:F+4])[0]
                lk=struct.unpack('>'+'H'*lcnt,d[F+4:F+4+2*lcnt])
                feats.append((ftag.decode(),lk))
        print(' ',tag.decode(), feats)
    lc=struct.unpack('>H',d[base+ll:base+ll+2])[0]
    types=[]
    for i in range(lc):
        o=struct.unpack('>H',d[base+ll+2+2*i:base+ll+4+2*i])[0]
        t,fl2,sc2=struct.unpack('>HHH',d[base+ll+o:base+ll+o+6])
        real=t
        if (T==b'GSUB' and t==7) or (T==b'GPOS' and t==9):
            so=struct.unpack('>H',d[base+ll+o+6:base+ll+o+8])[0]
            real=struct.unpack('>H',d[base+ll+o+so+2:base+ll+o+so+4])[0]
        types.append((i,real,fl2))
    print('  lookups',types)
