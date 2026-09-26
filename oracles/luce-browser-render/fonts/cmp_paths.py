import struct,sys
def parse(p):
    out=[]
    for l in open(p):
        parts=l.split()
        v=int(parts[0]); pts=[tuple(struct.unpack('f',struct.pack('f',float(x)))[0] for x in q.split(',')) for q in parts[1:]]
        out.append((v,pts))
    return out
a=parse(sys.argv[1]); b=parse(sys.argv[2])
for i,(x,y) in enumerate(zip(a,b)):
    if x!=y: print(i,x,y)
print(len(a),len(b))
