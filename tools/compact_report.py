"""Consolidate report illustrations while retaining the theory and object explanations."""
from pathlib import Path
from collections import defaultdict
from PIL import Image, ImageDraw, ImageFont, ImageOps


def compact_blocks(blocks, objects, materials, textures, texture_rules, out, path_for):
    diagrams=out/'diagrams'
    groups=[
        ('mesh-comparison',['wireframe','normals'],2,'Unit primitive construction, triangulated Buzz and transformed normals. Indexed triangles approximate curved surfaces in raster rendering.'),
        ('arrival-route',['garden','stairs'],2,'Penny in the garden at night (player-controlled prologue) and on the eighteen-step stair flight.'),
        ('rider-hierarchy',['bullseye','mounted'],2,'Bullseye and the mounted rider: ellipsoid anatomy, saddle attachment and articulated legs.'),
        ('desk-bed',['desk','bed'],2,'Desk and chair construction beside the bed frame, mattress, blanket and pillows.'),
        ('bookcase-poster',['bookcase','poster'],2,'Bookcase geometry with textured book rows, and the independently mapped wall poster.'),
        ('fan-clock',['fan','clock'],2,'Ceiling fan rotor and clock hands: distinct parent joints driven by scene time.'),
        ('ball-ghost',['ball','ghost'],2,'Rolling beach ball and translucent ghost: spherical UVs, accumulated rotation and time-dependent opacity.'),
        ('light-comparison',['directional','point','spot','ambient','diffuse','specular'],2,'Isolated directional, point and spot sources (top rows), followed by ambient, diffuse and specular terms. All views are captured from the application.'),
        ('shading-comparison',['flat','gouraud','phong','blinn'],2,'Matched Flat, Gouraud, Phong and Blinn-Phong views. The shading model changes while geometry, material and camera remain fixed.'),
        ('ray-comparison',['room-night','ray-zero','room-ray','house-ray'],2,'Raster room, zero-continuation primary view, two-continuation reflections and ray-traced exterior texture coverage.'),
        ('interaction-ending',['live-control','room-day','morning','story-end'],2,'Live character takeover, daytime lighting in the room, the morning pull-back over the house and free exploration outside.'),
    ]
    groups.extend([('puzzle-objects',['train-clue','clock-clue','block-clue','keypad'],2,'The four hallway puzzle objects: indexed primitive geometry, circular BMP face and raised keypad glyphs.'),('story-objects',['chest-closed','chest-open','toys-alive','buzz-room','wardrobe','rescue-jump'],3,"The closed toy chest, its opening lid, the toys alive, Buzz's bedroom, the closed wardrobe and Bullseye's jump."),('escape-objects',['buzz-flight','entrance-lock','door-impact','door-debris'],2,'Buzz flying out of the wardrobe, the locked main door, the nearest-hit laser impact and the board debris on the porch.')])
    lookup={name:(key,names,cols,caption) for key,names,cols,caption in groups for name in names}
    lookup.update({key:(key,names,cols,caption) for key,names,cols,caption in groups})
    seen=set();result=[]
    label_font=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf',23)
    def montage(key,names,cols,texture=False):
        tile_w=520 if not texture else 240; tile_h=310 if not texture else 220
        rows=(len(names)+cols-1)//cols
        sheet=Image.new('RGB',(cols*tile_w,rows*tile_h),(241,244,248));draw=ImageDraw.Draw(sheet)
        for i,name in enumerate(names):
            x=(i%cols)*tile_w;y=(i//cols)*tile_h
            source=Image.open(path_for(name,texture)).convert('RGB')
            image=ImageOps.contain(source,(tile_w-12,tile_h-42),Image.Resampling.LANCZOS)
            sheet.paste(image,(x+(tile_w-image.width)//2,y+(tile_h-42-image.height)//2))
            draw.text((x+10,y+tile_h-34),name.replace('-',' ').upper(),font=label_font,fill=(15,25,47))
        sheet.save(diagrams/(key+'.png'))
    introduction=True
    for block in blocks:
        if block[0]=='heading' and block[1].startswith('CHAPTER II'): introduction=False
        if introduction and block[0]=='figure' and block[1]=='room-night': continue
        if block[0]=='heading' and block[1].startswith('Appendix A'):break
        if block[0]=='figure' and block[1] in lookup:
            key,names,cols,caption=lookup[block[1]]
            if key not in seen:
                montage(key,names,cols);result.append(('figure',key,caption));seen.add(key)
        elif block[0]=='heading' and block[1].startswith('Surface map:'):continue
        elif block[0]=='paragraph' and block[1].startswith('Native size:'):continue
        elif block[0]=='texture':
            if 'texture-atlas' not in seen:
                names=[r['name'] for r in sorted(textures,key=lambda r:int(r['ray_layer']))]
                montage('texture-atlas',names,5,True)
                result.append(('figure','texture-atlas','All 27 mapped surfaces and the white fallback. Picket holes are shown against a checkerboard; they carry alpha coverage rather than geometric displacement.'))
                result.append(('table','Surface-map construction and use',['Map / layer','Construction','Use'],[
                    [r['name']+' / '+r['ray_layer']+'; '+r['width']+'x'+r['height'],texture_rules[r['name']][1],texture_rules[r['name']][0]] for r in sorted(textures,key=lambda r:int(r['ray_layer']))]))
                seen.add('texture-atlas')
        else:result.append(block)
    grouped=defaultdict(list)
    for row in objects:
        parts=row['path'].split('/'); group=parts[1] if len(parts)>1 else 'World'
        if group.startswith(('Clue','TrainClue','ToyRoomCombination')): group='Hallway puzzle mechanisms'
        elif group.startswith('ToyChest'): group='Toy chest'
        elif group.startswith('BuzzRoom'): group="Buzz's bedroom and wardrobe"
        elif group.startswith('DoorDebris'): group='Entrance debris'
        elif group.startswith('ContactShadow'): group='Character contact shadows'
        grouped[group].append(row)
    result.extend([
        ('heading','Appendix A - Object construction index',0),
        ('paragraph',f'The scene export contains {len(objects)} nodes, including {sum(r["primitive"]!="Joint" for r in objects)} mesh-bearing shapes. The construction groups below include hidden cinematic scenery, joints and contact proxies. Each leaf has local position, rotation, scale, material and UV repeats; its world matrix follows its parent chain. The companion objects.csv records every node and its actual transform, collision flags and geometry counts. Comprehensive-Implementation-Notes.md reproduces the complete construction tables and detailed model explanations.'),
        ('table','Complete scene coverage by construction group',['Group','Nodes / shapes','Primitive families'],[
            [name,str(len(rows))+' / '+str(sum(r['primitive']!='Joint' for r in rows)),', '.join(sorted(set(r['primitive'] for r in rows if r['primitive']!='Joint')))] for name,rows in grouped.items()]),
        ('heading','Appendix B - Material and study references',0),
        ('paragraph','Every material is recorded in materials.csv: colour, ambient/diffuse/specular coefficients, shininess, emission, opacity, mirror reflectivity, texture layer and UV scale. Representative coefficients below make the visual comparisons reproducible. Ghost opacity is a time-dependent value: 0.55 times visibility, or 0.55 while selected; the export records its initial hidden value of zero. The comprehensive notes retain the full material inventory. The Demonstration and Viva Guide supplies a timed two-minute narration, live-control rehearsal, theory questions and parameter-change practice. The numbered source documentation explains vertex/index construction, every object hierarchy, illumination, shading, textures and analytic ray tracing in greater depth.'),
        ('table','Representative actual material coefficients',['Material','ka/kd/ks','n','alpha / rho','UV'],[
            [r['name'],'/'.join(r[k] for k in ['ka','kd','ks']),r['shininess'],r['opacity']+' / '+r['reflectivity'],r['uv_x']+' / '+r['uv_y']] for r in materials if r['name'] in ['floor','beach-ball','ghost','lamp-metal','desk-wood','wall-side','car-glass','Buzz-helmet','brass','car-paint']]),
    ])
    return result
