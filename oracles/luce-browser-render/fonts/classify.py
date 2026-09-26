import collections
cpp=open('survey_cpp_all.txt').read().split('\n'); lu=open('survey_luce_all.txt').read().split('\n')
def blocks(lines):
    out={}; cur=None
    for l in lines:
        if l.startswith('/'):
            cur=l.split(':')[0]; out[cur]=[l]
        elif cur: out[cur].append(l)
    return out
a=blocks(cpp); b=blocks(lu)
kinds=collections.Counter(); fonts=collections.defaultdict(list)
for f in a:
    if f not in b: kinds['missing']+=1; continue
    for i,(x,y) in enumerate(zip(a[f],b[f])):
        if x!=y:
            k=('head' if i==0 else x.split(':')[0].strip().split(' ')[0]+(str(i) if 'shape' in x else ''))
            kinds[k]+=1; fonts[k].append(f.split('/')[-1])
print(kinds)
for k in fonts: print(k, fonts[k][:40])
