#!/usr/bin/env python3
"""Decodificador parcial de TBL/EVTPAL.BIN (scripts de eventos de etapa) de Vampire Night PAL.
EVTPAL se carga en RAM 0x1FC2000 (tabla ELF 0x26B2FC). EventCtrlTask (asm/EventCtrlTask_1E29F0.s, _1E2D10.s):
  pc = EVTPAL[etapa][evento][fase]   (tres niveles de punteros RAM; las listas de evento y de fase acaban en 0xFFFFFFFF)
  EVTPAL[0..12]: u32; 0..11 = punteros a la tabla de eventos de la etapa; [12] = 0x1FC2040 apunta a codigo, no a tabla.
  etapas 0..5 = fases 1..6, 6 = EVENT 7 (6 ev.), 7..9 = menus/clear (EVTPAL[8] = EventStageClear), 10 y 11 comparten tabla.
  Script = palabras u32: opcode, argumentos...; handler = ptmf de la tabla D_0026BC20 (12 B por opcode) que arma
  __sinit_eventctrl.cc a partir de plantillas del ELF 0x26B530 (16 B). NEXTEV avanza (fase+1 / evento).
Opcodes: solo se conoce parte (ver OPS y EVIDENCE). 53..110 = indice de plantilla + 4 (comprobado con __sinit y con la
estructura de los scripts de carga); 0..5, 7, 9..18, 20..28, 33..35, 37, 40 salen del emulador de __sinit; el resto sin resolver.
Uso: afs_evtpal.py   (resumen: tablas, colision por etapa, cobertura del desensamblado)"""
import struct, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
EVT = ROOT / "disc" / "TBL" / "EVTPAL.BIN"
BASE = 0x1FC2000
TEMPLATES = ['ENTB','ENTB2','ET1P','ET2P','ENTBSK','ZMAT','WLCTB','BRANCHNO','BRANCH','MISSENMCLR','SWITCH_MENM','BRANCHEV','SETEVNO','BRANCHADR','CCTB','CAMDR','GETCPMODE','WAIT_CAMJOB','WAIT_CAMFRM','WAIT_FRM','WAIT_ENNUM','WAIT_ENALV','WAIT_PEOPLE','WAIT_ITEM','WAIT_FLUSHDATA','WAIT','ITEMSET','BSFT','WAIT_INT_MODE','FOGCTRL','FOGCTRL','LIGHTCTRL','LIGHTCTRL','MLLIGHT','PLLIGHT','LIGHTIMMEXE','PUSHLIGHT','POPLIGHT','DOFCTRL','SCEFFINOUT','BLURCTRL','OVLAP','MOTVOLUME','MOTSHDWMAP','WAIT_BUFFERRAM','BGTB','FLRTB','SKYKD','SKYDSP','CHGSM','SCNINIT','NEXTEV','SKIP','BRANCHNOSET','SNOWDISP','DBG_MALLOC_PRINT','DBG_ENTDATA_INFO','JIMAKU','FORCE_CAMPARAM','VIBRATION','ENTRYSTGDATA','ENTRYCPBK','ENTRYBANK','FREEBANK','ENTRYSPR','FREESPR','ENTRYMOT','FREEMOT','ENTRYENVTX','FREEENVTX','ENTRYOSK','FREEOSK','ENTRYOSKNAME','FREEOSKNAME','ENTRYOSKBK','FREEOSKBK','ENTRYCLVTX','FREECLVTX','ENTRYFANI','FREEFANI','ENTRYFAMOT','FREEFAMOT','ENTRYCOLLI','FREECOLLI','FLUSHDATA','ENTRYTEX','FREETEX','NOWLOADINGSET','ENTRYLANGSPR','FREELANGSPR','ENTRYLANGBANK','FREELANGBANK','ENTRYSNDPK','FREESNDPK','BGMPLAY','VOICEPLAY','BGMPAUSE','BGMFADE','SKVOOFF','SNDON','SNDONM','SNDONSK','SNDONMSK','LOAD_OVLMOD','WAIT_LOAD_OVLMOD','CHGADXPT','JOBFLAGSET','BRANCHITEM','ENTRYSUBGIRL','GAMEFLAGSET','WAIT_FLUSHDATA_NOPAUSE']
OPS = {}; EVIDENCE = {}
for t in range(49, 107): OPS[t + 4] = TEMPLATES[t]; EVIDENCE[t + 4] = "plantilla+4"
for s in (0, 1, 2, 3, 4, 5): OPS[s] = TEMPLATES[s]; EVIDENCE[s] = "sinit"
for s, nm in {7:'WLCTB',9:'BRANCHNO',10:'BRANCH',11:'MISSENMCLR',12:'SWITCH_MENM',13:'BRANCHEV',14:'SETEVNO',15:'BRANCHADR',16:'CCTB',17:'CAMDR',18:'GETCPMODE',20:'WAIT_CAMJOB',21:'WAIT_CAMFRM',22:'WAIT_FRM',23:'WAIT_ENNUM',24:'WAIT_ENALV',25:'WAIT_PEOPLE',26:'WAIT_ITEM',27:'WAIT_FLUSHDATA',28:'WAIT',33:'FOGCTRL',34:'LIGHTCTRL',35:'LIGHTCTRL',37:'PLLIGHT',40:'POPLIGHT'}.items():
    OPS[s] = nm; EVIDENCE[s] = "sinit"
# palabras de argumento (de los incrementos de pc en los handlers); los no listados son de longitud variable o no medida
ARGS = {'WLCTB':0,'MLLIGHT':1,'PLLIGHT':1,'FLRTB':1,'SKYKD':1,'SKYDSP':1,'CHGSM':1,'SKIP':1,'SKVOOFF':0,'GETCPMODE':1,'ENTRYCPBK':1,'ENTRYSNDPK':3,'FREESNDPK':1,'BGMPLAY':5,'VOICEPLAY':5,'BGMPAUSE':1,'BGMFADE':3,'SNDON':4,'SNDONM':6,'SNDONSK':4,'SNDONMSK':6,'SNOWDISP':1,'WAIT_ENNUM':1,'WAIT_ENALV':1,'WAIT_PEOPLE':1,'WAIT_ITEM':1,'WAIT_FLUSHDATA':0,'WAIT_FLUSHDATA_NOPAUSE':0,'WAIT_BUFFERRAM':0,'LOAD_OVLMOD':1,'WAIT_LOAD_OVLMOD':1,'CHGADXPT':1,'DBG_MALLOC_PRINT':0,'DBG_ENTDATA_INFO':0,'ITEMSET':2,'NOWLOADINGSET':1,'OVLAP':4,'MOTVOLUME':4,'MOTSHDWMAP':2,'SCNINIT':0,'ENTRYSTGDATA':0,'FLUSHDATA':0,'LIGHTIMMEXE':1,'PUSHLIGHT':1,'POPLIGHT':1,'ZMAT':0,'BRANCHNO':1,'MISSENMCLR':0,'JOBFLAGSET':1,'JIMAKU':1,'BSFT':1,'WAIT_INT_MODE':1,'FORCE_CAMPARAM':3,'GAMEFLAGSET':2,'ENTRYSUBGIRL':0,'BRANCHNOSET':1,'WAIT_FRM':1,'NEXTEV':0}
for _nm in TEMPLATES:
    if _nm.startswith(('ENTRY', 'FREE')) and _nm not in ARGS and _nm not in ('ENTRYOSKNAME','FREEOSKNAME','ENTRYFANI','FREEFANI','ENTRYFAMOT','FREEFAMOT'): ARGS[_nm] = 1

class Evtpal:
    def __init__(self, path=EVT):
        self.d = Path(path).read_bytes(); self.n = len(self.d) // 4
        self.w = struct.unpack_from("<%dI" % self.n, self.d, 0)
    def isptr(self, x): return BASE <= x < BASE + len(self.d) and x % 4 == 0
    def scripts(self):
        """[(etapa, evento, fase, indice de palabra del pc)]"""
        out = []
        for s in range(12):
            o = (self.w[s] - BASE) // 4; ev = 0
            while self.isptr(self.w[o]):
                p = (self.w[o] - BASE) // 4; ph = 0
                while self.isptr(self.w[p]): out.append((s, ev, ph, (self.w[p] - BASE) // 4)); p += 1; ph += 1
                o += 1; ev += 1
        return out
    def walk(self, i, limit=4000):
        """Desensambla desde la palabra i mientras todos los opcodes tengan longitud conocida; -> (instrucciones, motivo de parada)."""
        out = []
        while len(out) < limit and i < self.n:
            op = self.w[i]; nm = OPS.get(op)
            if nm is None: return out, "opcode desconocido %d" % op
            if nm not in ARGS: return out, "argumentos de %s sin medir" % nm
            a = list(self.w[i + 1:i + 1 + ARGS[nm]]); out.append((i, nm, a)); i += 1 + ARGS[nm]
            if nm == 'NEXTEV': return out, "NEXTEV"
        return out, "limite"
    def colli_requests(self):
        """{etapa: argumentos de ENTRYCOLLI} localizados con el patron 'ENTRYCOLLI 0, ENTRYCOLLI k' (opcode 86) al inicio de cada evento."""
        res = {}
        for s, ev, ph, i in self.scripts():
            seq = self.w[i:i + 260]
            for j in range(len(seq) - 3):
                if seq[j] == 86 and seq[j + 1] == 0 and seq[j + 2] == 86 and seq[j + 3] < 8: res.setdefault(s, set()).add(seq[j + 3])
        return res

if __name__ == "__main__":
    e = Evtpal(); sc = e.scripts()
    print("palabras", e.n, "scripts (etapa,evento,fase)", len(sc), "pc distintos", len({x[3] for x in sc}))
    print("cabecera", [hex(x) for x in e.w[:13]])
    stop = {}
    for s, ev, ph, i in sc:
        ins, why = e.walk(i); stop[why.split(' ')[0] + ' ' + (why.split(' ')[-1] if 'desconocido' in why or 'sin' in why else '')] = stop.get(why.split(' ')[0] + ' ' + (why.split(' ')[-1] if 'desconocido' in why or 'sin' in why else ''), 0) + 1
    print("paradas del desensamblado:", sorted(stop.items(), key=lambda kv: -kv[1])[:12])
    print("ENTRYCOLLI por etapa:", {s: sorted(v) for s, v in sorted(e.colli_requests().items())})
