"""Compare actual NPC positions during reproducible live takeover of each toy."""
from pathlib import Path
import json, math, re, subprocess
ROOT=Path(__file__).resolve().parents[1]
LOG=ROOT/'docs/showcase/validation'
NAMES=['Woody','Jessie','Bullseye','Buzz','RC Car']
def capture(actor,frames,name,drive=0):
    args=[str(ROOT/'bin/Release/HauntedToyRoom.exe'),'--no-intro','--no-raytrace','--story','--gameplay-demo','--seek','30','--rehearsal-stop','--story-step','0.05','--frames',str(frames),'--size','640,360','--capture',str(LOG/(name+'.bmp'))]
    if actor is not None: args+=['--select',str(actor),'--drive',str(drive),'--turn','0.2']
    result=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,timeout=120)
    output=result.stdout+result.stderr
    (LOG/(name+'.log')).write_text(output,encoding='utf8')
    assert result.returncode==0,output
    positions={n:tuple(map(float,xyz.split(','))) for n,xyz in re.findall(r'^(Woody|Jessie|Bullseye|Buzz|RC Car) at ([\d.,e+\-]+)$',output,re.M)}
    assert len(positions)==5
    return output,positions
if __name__=='__main__':
    results=[]
    _,baseline=capture(None,40,'live-baseline')
    for actor,name in enumerate(NAMES):
        output,positions=capture(actor,40,'live-'+str(actor),0.35)
        assert 'Live control: '+name in output,name
        assert math.dist(positions[name],baseline[name])>.2,name+' ignored live input'
        differences={other:math.dist(positions[other],baseline[other]) for other in NAMES if other!=name}
        for other,delta in differences.items(): assert delta<.08,(name,other,delta)
        results.append({'owned':name,'owned_input_changes_actual_pose':True,'other_actor_position_differences':differences,'simulation_seconds':2})
        print('PASS live takeover:',name,flush=True)
    result=subprocess.run([str(ROOT/'bin/Release/HauntedToyRoom.exe'),'--no-intro','--story','--gameplay-demo','--no-raytrace','--seek','75','--frames','2','--size','640,360','--capture',str(LOG/'escape-win.bmp')],cwd=ROOT,capture_output=True,text=True,timeout=120)
    (LOG/'escape-win.log').write_text(result.stdout+result.stderr,encoding='utf8')
    assert result.returncode==0 and 'GAMEPLAY WIN / ENDING' in result.stdout
    assert 'ESCAPE real laser broke entrance door' in result.stdout
    actual=re.findall(r'^(Penny|Woody|Jessie|Bullseye|Buzz) at ([\d.,e+\-]+)',result.stdout,re.M)
    assert len(actual)==5
    for name,xyz in actual:
        x,y,z=map(float,xyz.split(','));assert y<-.3 and z>13,(name,xyz)
    results.append({'escape_rehearsal':'WIN','nearest_hit_door_impact':True,'all_five_actual_positions_outside':True})
    (LOG/'live-control.json').write_text(json.dumps(results,indent=2),encoding='utf8')
