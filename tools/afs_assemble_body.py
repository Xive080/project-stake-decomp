#!/usr/bin/env python3
"""Prueba: monta las piezas del cuerpo tipo 0 (tabla ELF 0x273CC0: cuerpo, parte) de ST5 banco 122 y las deforma de forma rigida con un movimiento de ST1 776/0.
Experimental: convencion de rotacion y pose de reposo del esqueleto frente a la T-pose de la malla sin resolver. Uso: afs_assemble_body.py [cuadro]"""
import sys,struct; sys.path.insert(0,'tools')
import numpy as np, afs, afs_export as X, ps2_model as M, afs_skeleton as S
from afs_unpack import parse_container
d=open('baserom/SCES_504.11','rb').read(); at=lambda v:v-0x140000+0x800
A=[struct.unpack_from('<I',d,at(0x26E880)+4*i)[0] for i in range(200)]
p=struct.unpack_from('<I',d,at(0x273CC0))[0]
ents=[]
for i in range(11):
    b,x,part,lst=struct.unpack_from('<4I',d,at(p)+16*(i+1)); ents.append((b,part))
f,E,_,_=afs.parse('disc/FS/ST5.AFS'); o,s=E[243]; f.seek(o); bl=parse_container(f.read(s))
def verts(x):
    v=[]
    for k,tex,cm in X.packets_by_material(x):
        nv=cm[3][1]; p3=cm[3][3]
        v+= [struct.unpack_from('<3f',x,p3+12*i) for i in range(nv)]
    return np.array(v)
par,off=S.skeleton(0); n=len(par)
P=np.zeros((n,3))
for i in range(n):
    P[i]=(P[par[i]] if par[i]>=0 else 0)+off[i]
print('joints',np.round(P,2).tolist())
for b,part in ents:
    blk=A[b]&0xffff; v=verts(bl[blk]); print('body',b,'part',part,'blk',blk,'n',len(v),'ctr',np.round(v.mean(0),2),'ext',np.round(v.max(0)-v.min(0),2),'joint',np.round(P[part],2))

import afs_motion as AM
from PIL import Image, ImageDraw
f1,E1,_,_=afs.parse('disc/FS/ST1.AFS'); f1.seek(E1[776][0]); mb=f1.read(E1[776][1]); sub=AM.subfiles(mb)[0]
tr,ang=AM.frames(sub,18); print('frames',len(tr))
pieces=[(part,verts(bl[A[b]&0xffff])) for b,part in ents[:10]]
def deform(fr,order,flip=1.0):
    n=18; Pn=np.zeros((n,3)); R=[None]*n
    for i in range(n):
        Ri=S.euler(ang[fr][i],order)
        if par[i]<0: R[i]=Ri; Pn[i]=tr[fr]*0
        else: R[i]=R[par[i]]@Ri; Pn[i]=Pn[par[i]]+R[par[i]]@(off[i]*flip)
    out=[]
    for part,v in pieces: out.append(Pn[part]+(v-P[part])@R[part].T)
    return np.vstack(out)
orders=['zyx','xyz','yxz','zxy']
fr=int(sys.argv[1]) if len(sys.argv)>1 else 20
img=Image.new('RGB',(4*300,600),'white'); dr=ImageDraw.Draw(img)
for k,o in enumerate(orders):
    v=deform(fr,o)
    for x,y,z in v: dr.point((k*300+150+x*14,300-y*14),fill=(0,0,0))
    dr.text((k*300+5,5),o,fill=(255,0,0))
img.save('build/export/pose_orders.png')

ang0=ang.copy(); ang[:]=0
img=Image.new('RGB',(900,300),'white'); dr=ImageDraw.Draw(img)
v=deform(0,'zyx')
for x,y,z in v: dr.point((150+x*14,150-y*14),fill=(0,0,0))
dr.text((5,5),'bind',fill=(255,0,0))
ang[:]=ang0
for k,fr in enumerate((0,40)):
    v=deform(fr,'zyx')
    for x,y,z in v: dr.point((300*(k+1)+150+x*14,150-y*14),fill=(0,0,0))
    dr.text((300*(k+1)+5,5),'frame %d'%fr,fill=(255,0,0))
print(np.round(ang0[0][:5],2)); img.save('build/export/pose_bind.png')
