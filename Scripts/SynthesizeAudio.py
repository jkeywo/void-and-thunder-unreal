from pathlib import Path
import math,wave,struct
RATE=22050
class Noise:
 def __init__(self,seed):self.seed=seed
 def next(self):
  self.seed=(self.seed*1664525+1013904223)&0xffffffff
  return (self.seed>>8)/(1<<24)*2-1
def render(name,duration,seed=1):
 noise=Noise(seed); lp=0; samples=[]
 for i in range(int(RATE*duration)):
  t=i/RATE; q=t/duration; sine=lambda freq:math.sin(t*freq*math.tau)
  if name=='broadside':value=(sine(120-70*q)*.6+noise.next()*.4)*math.exp(-t*12)*.6
  elif name=='hit':value=(1 if sine(760)>=0 else -1)*math.exp(-t*40)*.4
  elif name=='explosion':lp+=(noise.next()-lp)*.2; value=lp*math.exp(-t*6)*.7
  elif name=='board':value=sine(523 if t<.12 else 784)*math.exp(-(t if t<.12 else t-.12)*8)*.4
  elif name=='boost':lp+=(noise.next()-lp)*(.04+.30*q); value=(lp*.7+sine(90+260*q)*.3)*min(q*4,1)*(1-q)**1.5*.5
  elif name=='warp':value=(sine(900*(1-q*.85)+60)*.55+(noise.next()*.5 if t<.03 else 0))*math.exp(-t*7)*.45
  elif name=='brace':value=(sine(320)*.6+sine(487)*.4)*math.exp(-t*18)*.4
  else:value=sine(196)*.3*(math.exp(-t*11) if t<.13 else math.exp(-(t-.15)*11)*(0 if t<.15 else 1))
  samples.append(int(max(-1,min(1,value))*32767))
 destination=Path(__file__).resolve().parents[1]/'SourceAssets'/'audio'; destination.mkdir(exist_ok=True)
 with wave.open(str(destination/(name+'.wav')),'wb') as f:f.setnchannels(1); f.setsampwidth(2); f.setframerate(RATE); f.writeframes(struct.pack('<'+'h'*len(samples),*samples))
for args in [('broadside',.28,0x2468),('hit',.09),('explosion',.5,0x1357),('board',.34),('boost',.45,0x9abc),('warp',.38,0x5e1f),('brace',.22),('hullwarn',.30)]:render(*args)
