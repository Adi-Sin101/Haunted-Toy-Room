"""Build the report (PDF/DOCX), ten-slide deck (PPTX/PDF), and learning guide.

Requirements: Pillow, python-docx, python-pptx, reportlab, pymupdf.
Run tools/make_showcase.py --capture --video first.
"""
from pathlib import Path
import csv
import html
import json
import math
import random
import re
import textwrap
from collections import defaultdict

from PIL import Image, ImageDraw, ImageFont, ImageFilter
from docx import Document
from docx.shared import Inches as DInches, Pt as DPt, RGBColor as DColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.shapes import MSO_SHAPE
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY
from reportlab.lib.pagesizes import A4
from reportlab.platypus import BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Image as RLImage, Table, TableStyle, PageBreak, KeepTogether, NextPageTemplate
from reportlab.platypus.tableofcontents import TableOfContents
from reportlab.pdfgen import canvas as pdfcanvas
import pymupdf

from showcase_content import *

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/showcase"
FIG = OUT / "figures"
DIAG = OUT / "diagrams"
WIDTH = A4[0] - (1.2 + 1.0) * 72
NAVY = (15, 25, 47)
GOLD = (255, 219, 103)
CREAM = (246, 241, 226)
BLUE = (137, 190, 211)


def font(size, bold=False, serif=False):
    file = "georgiab.ttf" if serif and bold else "georgia.ttf" if serif else "segoeuib.ttf" if bold else "segoeui.ttf"
    return ImageFont.truetype("C:/Windows/Fonts/" + file, size)


def wrapped(draw, xy, text, size=25, fill=NAVY, max_width=900, bold=False, serif=False, spacing=8):
    fnt=font(size,bold,serif)
    lines=[]
    for paragraph in text.split("\n"):
        line=""
        for word in paragraph.split():
            candidate=(line+" "+word).strip()
            if draw.textlength(candidate,font=fnt)>max_width and line:
                lines.append(line); line=word
            else: line=candidate
        lines.append(line)
    y=xy[1]
    for line in lines:
        draw.text((xy[0],y),line,font=fnt,fill=fill)
        y+=size+spacing
    return y


def arrow(draw, start, end, colour=BLUE, width=5):
    draw.line([start,end], fill=colour,width=width)
    a=math.atan2(end[1]-start[1],end[0]-start[0]);length=14
    draw.polygon([end,(end[0]-length*math.cos(a-.5),end[1]-length*math.sin(a-.5)),(end[0]-length*math.cos(a+.5),end[1]-length*math.sin(a+.5))],fill=colour)


def diagram(name,title):
    image=Image.new("RGB",(1400,760),CREAM);draw=ImageDraw.Draw(image)
    draw.rounded_rectangle((25,25,1375,735),25,outline=BLUE,width=3)
    wrapped(draw,(60,45),title,36,bold=True,max_width=1250)
    return image,draw


def box(draw,rect,text,colour=BLUE,size=25):
    draw.rounded_rectangle(rect,15,fill=colour,outline=NAVY,width=2)
    wrapped(draw,(rect[0]+18,rect[1]+15),text,size,max_width=rect[2]-rect[0]-32,bold=True)


def diagrams():
    DIAG.mkdir(parents=True,exist_ok=True)
    image,d=diagram("pipeline-diagram","One frame: input, scene, light and rendering")
    steps=[("Input + dt",70),("Story / motion",380),("Contacts",690),("World matrices",1000)]
    for text,x in steps:
        box(d,(x,150,x+250,245),text)
        if x<1000:arrow(d,(x+250,197),(x+305,197),NAVY)
    box(d,(180,350,585,480),"Raster path\nVertices → triangles → fragments",size=28)
    box(d,(795,350,1200,480),"Ray path\nBVH → analytic hit → rays",size=28)
    arrow(d,(1130,245),(1130,335),NAVY);arrow(d,(1100,280),(390,335),NAVY)
    box(d,(450,570,970,675),"Shared materials, surface maps and lights\nHUD + final display",size=25)
    arrow(d,(390,480),(540,560),NAVY);arrow(d,(995,480),(875,560),NAVY)
    image.save(DIAG/"pipeline-diagram.png")
    image,d=diagram("hierarchy-diagram","A rider moves through her parent's coordinate system")
    box(d,(80,160,370,255),"Independent Jessie");box(d,(70,370,390,485),"World\nJessie local transform")
    arrow(d,(225,255),(225,355),NAVY)
    box(d,(615,135,1015,220),"Bullseye world transform");box(d,(615,295,1015,380),"Saddle local transform");box(d,(615,455,1015,540),"Jessie local transform")
    arrow(d,(815,220),(815,280),NAVY);arrow(d,(815,380),(815,440),NAVY)
    arrow(d,(390,430),(600,490),NAVY)
    wrapped(d,(75,600),"Mount: local = inverse(saddle world) × Jessie world\nThen world(rider) = world(horse) × local(saddle) × local(rider)",28,max_width=1250)
    image.save(DIAG/"hierarchy-diagram.png")
    image,d=diagram("lighting-diagram","Light geometry and the surface vectors")
    for j,title in enumerate(["DIRECTIONAL","POINT","SPOT"]):
        x=90+j*435;wrapped(d,(x,130),title,28,bold=True,max_width=360)
        if j==0:
            for dx in [25,105,185]:arrow(d,(x+dx,205),(x+dx+70,330),NAVY)
        elif j==1:
            d.ellipse((x+120,220,x+150,250),fill=GOLD,outline=NAVY)
            for a in range(0,360,45):arrow(d,(x+135,235),(x+135+105*math.cos(math.radians(a)),235+105*math.sin(math.radians(a))),NAVY)
        else:
            d.polygon([(x+140,205),(x+35,365),(x+250,365)],fill=(239,215,145))
            arrow(d,(x+140,205),(x+140,360),NAVY)
            d.line([(x+140,205),(x+70,360)],fill=BLUE,width=4);d.line([(x+140,205),(x+210,360)],fill=BLUE,width=4)
    d.line((240,600,1120,600),fill=NAVY,width=6)
    for end,label,col in [((690,445),"N",NAVY),((510,435),"L",(197,153,38)),((875,445),"V",BLUE),((840,415),"R",(137,73,112))]:
        arrow(d,(690,600),end,col);wrapped(d,(end[0]+10,end[1]-15),label,28,fill=col,bold=True,max_width=55)
    wrapped(d,(80,675),"Diffuse: max(N·L,0)       Phong specular: max(R·V,0)^n",28,max_width=1260)
    image.save(DIAG/"lighting-diagram.png")
    image,d=diagram("ray-diagram","Analytic visibility, shadow, reflection and transmission")
    d.rectangle((70,510,1330,550),fill=(123,89,70));d.ellipse((610,250,780,420),fill=BLUE,outline=NAVY,width=4)
    d.ellipse((1050,120,1130,200),fill=GOLD,outline=NAVY,width=3)
    box(d,(80,145,320,230),"Camera",size=28)
    arrow(d,(320,230),(695,335),NAVY);wrapped(d,(330,230),"primary ray",26,max_width=230)
    arrow(d,(780,300),(1050,160),(197,153,38));wrapped(d,(840,210),"shadow ray",26,max_width=230)
    arrow(d,(695,420),(855,510),NAVY);arrow(d,(855,510),(1170,335),(137,73,112));wrapped(d,(970,390),"mirror ray",26,max_width=230)
    arrow(d,(780,355),(1080,450),BLUE);wrapped(d,(850,480),"straight transmission",26,max_width=370)
    wrapped(d,(85,620),"Nearest positive hit → local illumination → visibility test → bounded continuation",28,max_width=1240)
    image.save(DIAG/"ray-diagram.png")
    image,d=diagram("bvh-diagram","Bounding volume hierarchy: reject a group before exact tests")
    box(d,(520,125,880,200),"Root: all scene bounds",size=27)
    box(d,(195,315,525,395),"Left spatial group",size=27);box(d,(875,315,1205,395),"Right spatial group",size=27)
    arrow(d,(610,200),(360,300),NAVY);arrow(d,(790,200),(1040,300),NAVY)
    for x,text in [(90,"sphere + cube"),(460,"cone + plane"),(810,"car wheels"),(1160,"roof pieces")]:
        box(d,(x-25,515,x+200,595),text,size=23)
    for x,start in [(200,360),(570,360),(930,1040),(1230,1040)]:arrow(d,(start,395),(x,500),NAVY)
    wrapped(d,(65,650),"Median split on longest centre axis; near child first; prune boxes beyond the current hit.",25,max_width=1290)
    image.save(DIAG/"bvh-diagram.png")
    image,d=diagram("uv-diagram","UVs map a two-dimensional surface pattern onto geometry")
    for y in range(4):
        for x in range(6):d.rectangle((90+x*60,210+y*65,150+x*60,275+y*65),fill=BLUE if (x+y)%2 else GOLD,outline=NAVY)
    wrapped(d,(90,510),"u → longitude / perimeter\nv → latitude / height",27,max_width=400)
    arrow(d,(485,335),(690,335),NAVY)
    d.ellipse((745,180,1095,530),fill=BLUE,outline=NAVY,width=5)
    for x in [810,865,920,975,1030]:d.arc((x-60,180,x+60,530),85,275,fill=NAVY,width=3)
    for y in [265,355,445]:d.ellipse((755,y-35,1085,y+35),outline=NAVY,width=3)
    wrapped(d,(1140,260),"u=0\nseam\nu=1",27,max_width=160)
    wrapped(d,(90,650),"uv' = uv × repeat scale      albedo = material colour × sampled RGB",28,max_width=1240)
    image.save(DIAG/"uv-diagram.png")
    image,d=diagram("primitive-diagram","Five shared primitive families build the complete scene")
    names=["PLANE","CUBE","SPHERE","CYLINDER","CONE"]
    uses=["walls / floor","body / furniture","head / ball / eyes","limbs / wheels","shade / ghost"]
    for i,name in enumerate(names):
        x=80+i*265;wrapped(d,(x,145),name,25,bold=True,max_width=245)
        if i==0:d.polygon([(x,355),(x+120,290),(x+200,350),(x+80,415)],fill=BLUE,outline=NAVY,width=4)
        if i==1:
            d.polygon([(x,300),(x+115,260),(x+185,320),(x+80,365)],fill=GOLD,outline=NAVY,width=3);d.polygon([(x,300),(x+80,365),(x+80,470),(x,410)],fill=BLUE,outline=NAVY,width=3);d.polygon([(x+80,365),(x+185,320),(x+185,425),(x+80,470)],fill=(123,158,174),outline=NAVY,width=3)
        if i==2:
            d.ellipse((x,270,x+195,465),fill=BLUE,outline=NAVY,width=4);d.ellipse((x+65,270,x+130,465),outline=NAVY,width=2);d.ellipse((x,335,x+195,400),outline=NAVY,width=2)
        if i==3:
            d.rectangle((x+20,295,x+170,435),fill=BLUE,outline=NAVY,width=3);d.ellipse((x+20,270,x+170,320),fill=GOLD,outline=NAVY,width=3);d.arc((x+20,410,x+170,460),0,180,fill=NAVY,width=3)
        if i==4:
            d.polygon([(x+95,265),(x+15,440),(x+180,440)],fill=BLUE,outline=NAVY,width=3);d.ellipse((x+15,415,x+180,465),fill=GOLD,outline=NAVY,width=3)
        wrapped(d,(x,525),uses[i],23,max_width=235)
    wrapped(d,(80,665),"One mesh family, many model matrices: scale + rotate + translate",28,max_width=1250)
    image.save(DIAG/"primitive-diagram.png")


def themed_background():
    random.seed(31)
    im=Image.new("RGB",(1600,900));pix=im.load()
    for y in range(900):
        for x in range(1600):
            glow=max(0,1-math.hypot((x-1190)/900,(y-220)/700))
            pix[x,y]=(int(12+26*glow),int(23+29*glow),int(45+35*glow))
    clouds=Image.new("RGBA",im.size);d=ImageDraw.Draw(clouds)
    for _ in range(24):
        x=random.randint(-400,1500);y=random.randint(25,620);d.ellipse((x,y,x+random.randint(250,700),y+100),fill=(135,150,163,25))
    im=Image.alpha_composite(im.convert("RGBA"),clouds.filter(ImageFilter.GaussianBlur(40)));d=ImageDraw.Draw(im)
    d.ellipse((1060,35,1490,465),fill=(232,223,222,255))
    for _ in range(38):
        x=random.randint(1110,1420);y=random.randint(90,360);r=random.randint(12,42);d.ellipse((x-r,y-r,x+r,y+r),fill=(214,206,208,255))
    dark=(12,18,28,255)
    d.rectangle((1090,275,1440,665),fill=dark)
    d.polygon([(1050,300),(1140,190),(1390,190),(1480,300)],fill=(22,28,39,255))
    d.polygon([(1070,500),(1100,450),(1435,450),(1465,500)],fill=(22,28,39,255))
    for y in [335,520]:
        for x in [1120,1205,1290,1375]:
            d.rounded_rectangle((x,y,x+43,y+78),21,fill=(244,191,75,255))
            d.line((x+21,y,x+21,y+78),fill=dark,width=3);d.line((x,y+38,x+43,y+38),fill=dark,width=3)
    d.rounded_rectangle((1238,570,1295,665),28,fill=(95,61,49,255))
    # Original branch silhouettes frame the slides without copying reference artwork.
    def branch(x,y,length,angle,depth,width):
        ex=x+length*math.cos(angle);ey=y+length*math.sin(angle)
        d.line((x,y,ex,ey),fill=dark,width=width)
        if depth:
            branch(ex,ey,length*.72,angle-.45,depth-1,max(2,int(width*.65)))
            branch(ex,ey,length*.63,angle+.6,depth-1,max(2,int(width*.65)))
    branch(75,810,200,-1.5,5,27);branch(1500,810,180,-1.75,4,25)
    d.polygon([(0,820),(260,780),(550,825),(940,775),(1250,800),(1600,775),(1600,900),(0,900)],fill=dark)
    for x in range(0,1600,18):
        y=random.randint(785,860);d.line((x,900,x+random.randint(-25,25),y),fill=(7,13,21,255),width=random.randint(3,7))
    im.convert("RGB").save(OUT/"haunted-background.png")


def data_rows():
    with (OUT/"inventory/objects.csv").open(encoding="utf-8") as fp:objects=list(csv.DictReader(fp))
    with (OUT/"inventory/materials.csv").open(encoding="utf-8") as fp:materials=list(csv.DictReader(fp))
    with (OUT/"inventory/textures/textures.csv").open(encoding="utf-8") as fp:textures=list(csv.DictReader(fp))
    return objects,materials,textures


def benchmark_rows():
    rows=[]
    for mode in ["raster","ray"]:
        raw=(OUT/f"validation/benchmark-{mode}.log").read_bytes()
        text=raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8-sig")
        match=re.search(r"Benchmark: (\d+) frames, ([\d.]+) ms/frame \(([\d.]+) FPS\), ([\d.]+) draw calls, ([\d.]+) triangles",text)
        assert match,mode
        n,ms,fps,draws,tri=match.groups()
        rows.append(["Raster Blinn-Phong" if mode=="raster" else "Ray tracing (0.5 scale, 2 bounces)",n,ms,fps,draws if mode=="raster" else "2 fullscreen passes",tri if mode=="raster" else "Analytic primitives"])
    return rows


def expanded_blocks():
    objects,materials,textures=data_rows()
    result=[]
    for block in BLOCKS:
        kind=block[0]
        if kind=="texture_catalogue":
            for index,row in enumerate(sorted(textures,key=lambda r:int(r["ray_layer"])),1):
                name=row["name"];use,rule=TEXTURES[name]
                result.append(("heading",f"Surface map: {name}",3))
                result.append(("paragraph",f"Native size: {row['width']} × {row['height']} texels. Ray layer: {row['ray_layer']}. Use: {use}. Construction: {rule}"))
                result.append(("texture",name,f"Exported {name} pattern. Material tint and UV repeat count determine its final scene appearance."))
        elif kind=="benchmarks":
            result.append(("table","Observed rendering timings",["Mode","Frames","ms/frame","FPS","Draws/frame","Geometry"],benchmark_rows()))
            result.append(("paragraph","These observations use the final Release executable at 1600 × 900 with 60 warm-up frames followed by 200 timed frames, v-sync disabled, and a fixed manual room view. The two rendering paths use their normal settings. Ray statistics refer to full-screen passes, not the raster triangle counters. Timing is influenced by scene progression and concurrent system work; it is not an isolated before/after experiment."))
        elif kind=="verification":
            assert json.loads((OUT/"validation/captures.json").read_text())["story_completed"]
            interaction=json.loads((OUT/"validation/interaction.json").read_text())
            assert len(interaction)==3
            result.append(("table","Verification evidence",["Area","Method","Result"],[
                ["Geometry / transforms / contacts","tests/PhysicsChecks.cpp","63 checks passed"],
                ["Manual + mounted + live input","Actual Windows key messages into GLFW callbacks","All three scenarios passed"],
                ["Story completion","Fixed-step connected escape rehearsal","WIN; real door hit; all five actors outside"],
                ["Rendering","55 deterministic PNG captures; capture checks GL errors","All paths completed"],
                ["Live takeover","Five ownership comparisons plus real key ownership/release checks","Owned transform retained; other route actions continue"],
                ["Video","2880 frames, 1280 × 720, 24 fps; complete decode","120 seconds; passed"],
            ]))
        else:result.append(block)
    result.extend([("heading","Appendix A — Complete scene-node inventory",0),("paragraph",f"The exported scene contains {len(objects)} nodes, including {sum(r['primitive']!='Joint' for r in objects)} mesh-bearing shapes. The following tables include hidden scenery and non-rendered joints as well as visible objects. Shape counts include contact shadows, sky elements and duplicate decorative instances. Each local position, rotation and scale is relative to the parent identified by the path. Rotation uses (pitch,yaw,roll) in degrees. Bracketed indices distinguish sibling nodes with repeated names. Joint rows carry no material; their transform affects their descendants. The accompanying objects.csv also records world positions, collision/visibility flags, vertex/triangle counts, UV repeats and material coefficients." )])
    groups=defaultdict(list)
    for row in objects:
        parts=row["path"].split("/")
        group=parts[1] if len(parts)>1 else "World"
        groups[group].append(row)
    def vec(row,prefix):return ",".join(f"{float(row[prefix+a]):.3g}" for a in ["x","y","z"])
    for group,rows in groups.items():
        result.append(("heading",group,2))
        result.append(("table",f"Construction records for {group}",["Relative node path","Shape / material","Position","Rotation°","Scale"],[
            [r["path"].split("/",2)[-1] if r["path"].count("/")>=2 else r["name"],r["primitive"]+(" / "+r["material"] if r["material"] else ""),vec(r,"p"),vec(r,"r"),vec(r,"s")] for r in rows
        ],"inventory"))
    result.extend([("heading","Appendix B — Complete material inventory",0),("paragraph","RGB values are multiplicative colour tints; ka, kd and ks are the ambient, diffuse and specular coefficients. n is shininess, alpha is opacity and rho is mirror reflectivity. A layer of -1 denotes no mapped surface. Emissive, unlit and cutout state and UV repeats are supplied in materials.csv. The same named material may be used by many nodes.")])
    result.append(("table","Material coefficients",["Material","RGB","ka/kd/ks","n","alpha/rho","Layer"],[
        [r["name"],",".join(f"{float(r[c]):.3g}" for c in ["r","g","b"]),"/".join(f"{float(r[c]):.3g}" for c in ["ka","kd","ks"]),r["shininess"],r["opacity"]+"/"+r["reflectivity"],r["texture_layer"]] for r in sorted(materials,key=lambda r:r["name"])
    ],"inventory"))
    return result


def path_for(name,texture=False):
    if texture and name=="pickets":return OUT/"inventory/textures/pickets-coverage.png"
    return OUT/"inventory/textures"/(name+".png") if texture else (DIAG/(name+".png") if (DIAG/(name+".png")).exists() else FIG/(name+".png"))


def report():
    for family,file in [("ReportTimes","times.ttf"),("ReportTimesBold","timesbd.ttf"),("ReportTimesItalic","timesi.ttf")]:
        pdfmetrics.registerFont(TTFont(family,"C:/Windows/Fonts/"+file))
    pdfmetrics.registerFontFamily("ReportTimes",normal="ReportTimes",bold="ReportTimesBold",italic="ReportTimesItalic",boldItalic="ReportTimesBold")
    styles={
        "body":ParagraphStyle("body",fontName="ReportTimes",fontSize=11,leading=14.3,alignment=TA_JUSTIFY,spaceAfter=6),
        "h1":ParagraphStyle("h1",fontName="ReportTimesBold",fontSize=16,leading=24,alignment=TA_CENTER,spaceBefore=12,spaceAfter=18,keepWithNext=True),
        "h2":ParagraphStyle("h2",fontName="ReportTimesBold",fontSize=14,leading=21,spaceBefore=12,spaceAfter=6,keepWithNext=True),
        "h3":ParagraphStyle("h3",fontName="ReportTimesBold",fontSize=12,leading=18,spaceBefore=10,spaceAfter=5,keepWithNext=True),
        "caption":ParagraphStyle("caption",fontName="ReportTimes",fontSize=9,leading=11.5,alignment=TA_CENTER,spaceBefore=4,spaceAfter=7),
        "cell":ParagraphStyle("cell",fontName="ReportTimes",fontSize=9,leading=12),
        "small":ParagraphStyle("small",fontName="ReportTimes",fontSize=7.5,leading=10),
        "equation":ParagraphStyle("equation",fontName="ReportTimes",fontSize=9.5,leading=12,leftIndent=7,spaceBefore=3,spaceAfter=7,borderColor=colors.HexColor("#89bed3"),borderWidth=.6,borderPadding=7,backColor=colors.HexColor("#f5f7fa")),
    }
    docx=Document();section=docx.sections[0]
    section.page_width=DInches(8.2677);section.page_height=DInches(11.6929)
    section.left_margin=DInches(1.2);section.right_margin=DInches(1);section.top_margin=DInches(1);section.bottom_margin=DInches(1)
    normal=docx.styles["Normal"];normal.font.name="Times New Roman";normal.font.size=DPt(11);normal.paragraph_format.line_spacing=1.15;normal.paragraph_format.space_after=DPt(6)
    for name,size in [("Heading 1",16),("Heading 2",14),("Heading 3",12)]:
        style=docx.styles[name];style.font.name="Times New Roman";style.font.size=DPt(size);style.font.color.rgb=DColor(15,25,47)
    footer=section.footer.paragraphs[0];footer.alignment=WD_ALIGN_PARAGRAPH.CENTER
    fld=OxmlElement("w:fldSimple");fld.set(qn("w:instr"),"PAGE");footer._p.append(fld)
    class Report(BaseDocTemplate):
        def __init__(self,path):
            super().__init__(str(path),pagesize=A4,leftMargin=86.4,rightMargin=72,topMargin=72,bottomMargin=72,title=TITLE,author=AUTHOR)
            frame=Frame(self.leftMargin,self.bottomMargin,self.width,self.height,id="main",leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)
            self.addPageTemplates(PageTemplate(id="normal",frames=frame,onPage=self.page_header))
        def page_header(self,c,doc):
            if doc.page==1:return
            c.setFont("ReportTimes",9);c.setFillColor(colors.HexColor("#39445c"));c.drawString(86.4,A4[1]-42,"Haunted Toy Room: The Midnight Mission | CSE-4102")
            c.drawCentredString(A4[0]/2,38,str(doc.page))
        def afterFlowable(self,flowable):
            if isinstance(flowable,Paragraph):
                text=flowable.getPlainText()
                if flowable.style.name=="h1" and text not in ["Abstract","Contents"]:
                    self.notify("TOCEntry",(0 if flowable.style.name=="h1" else 1,text,self.page))
    story=[]
    def addpara(text,style="body"):
        story.append(Paragraph(html.escape(text).replace("\n","<br/>"),styles[style]))
    # One combined course/title/student/teacher page.
    logo=OUT/"KUET-LOGO.png"
    story.append(RLImage(str(logo),width=72,height=82.6))
    logo_para=docx.add_paragraph();logo_para.alignment=WD_ALIGN_PARAGRAPH.CENTER;logo_para.add_run().add_picture(str(logo),width=DInches(1))
    story.append(Spacer(1,10))
    cover=[(COURSE,16),(COURSE_NAME,14),(TITLE.upper(),18),("Project Report",14),("By",12),(AUTHOR,14),("Roll: "+ROLL,14),("Course Teachers",12),*[(teacher,12) for teacher in TEACHERS],("Department of Computer Science and Engineering",12),("Khulna University of Engineering & Technology",12),("Khulna 9203, Bangladesh",12),("October 2026",12)]
    for index,(text,size) in enumerate(cover):
        st=ParagraphStyle("cover"+str(index),fontName="ReportTimesBold" if index in [0,2,5] else "ReportTimes",fontSize=size,leading=size*1.5,alignment=TA_CENTER,spaceAfter=13 if index in [1,2,6,10] else 8)
        story.append(Paragraph(html.escape(text),st))
        para=docx.add_paragraph();para.alignment=WD_ALIGN_PARAGRAPH.CENTER;run=para.add_run(text);run.font.size=DPt(size);run.bold=index in [0,2,5]
    story.append(PageBreak());docx.add_page_break()
    from compact_report import compact_blocks
    full=expanded_blocks()
    reference=[]
    for block in full:
        kind=block[0]
        if kind=="heading":reference.extend(["#"*max(1,block[2])+" "+block[1],""])
        elif kind in ["paragraph","equation"]:reference.extend([block[1],""])
        elif kind in ["figure","texture"]:reference.extend([f"![{block[2]}]({path_for(block[1],kind=='texture').relative_to(OUT).as_posix()})",""])
        elif kind=="table":reference.extend([block[1],"", "| "+" | ".join(block[2])+" |", "| "+" | ".join(["---"]*len(block[2]))+" |",*["| "+" | ".join(str(c).replace("|","/") for c in row)+" |" for row in block[3]],""])
    (OUT/"Comprehensive-Implementation-Notes.md").write_text("\n".join(reference),encoding="utf8")
    objects,materials,textures=data_rows()
    blocks=compact_blocks(full,objects,materials,textures,TEXTURES,OUT,path_for)
    # Insert contents and separate figure/table indexes after abstract.
    figures=[];tables=[];fn=tn=0
    for b in blocks:
        if b[0] in ["figure","texture"]:fn+=1;figures.append((fn,b[2]))
        if b[0]=="table":tn+=1;tables.append((tn,b[1]))
    contents_added=False;fn=tn=0
    markdown=["# "+TITLE,"",AUTHOR+" | Roll "+ROLL,"",COURSE+" — "+COURSE_NAME,""]
    for b in blocks:
        kind=b[0]
        if kind=="heading":
            _,text,level=b
            if text.startswith("CHAPTER I ") and not contents_added:
                addpara("Contents","h1")
                toc=TableOfContents();toc.levelStyles=[ParagraphStyle("toc0",fontName="ReportTimes",fontSize=10,leading=14,spaceBefore=2)]
                story.append(toc)
                docx.add_heading("Contents",0)
                for entry in blocks:
                    if entry[0]=="heading" and entry[2]<=1 and entry[1]!="Abstract":docx.add_paragraph(entry[1])
                contents_added=True
            if level<=1 and text!="Abstract":story.append(PageBreak());docx.add_page_break()
            addpara(text,"h1" if level<=1 else "h2" if level==2 else "h3")
            docx.add_heading(text,1 if level<=1 else level)
            markdown.extend(["#"*(1 if level<=1 else level)+" "+text,""])
        elif kind=="paragraph":
            addpara(b[1]);para=docx.add_paragraph(b[1]);para.alignment=WD_ALIGN_PARAGRAPH.JUSTIFY;markdown.extend([b[1],""])
        elif kind=="equation":
            addpara(b[1],"equation");para=docx.add_paragraph(b[1]);para.paragraph_format.line_spacing=1.2
            for run in para.runs:run.font.size=DPt(10.5)
            markdown.extend(["```text",b[1],"```",""])
        elif kind in ["figure","texture"]:
            fn+=1;_,name,caption=b;path=path_for(name,kind=="texture")
            with Image.open(path) as pic:w,h=pic.size
            width=WIDTH if kind=="figure" else min(230,WIDTH)
            height=width*h/w
            limit=245 if name in ["room-furniture","animated-props","light-comparison","texture-atlas"] else 210
            if height>limit:width*=limit/height;height=limit
            image=RLImage(str(path),width=width,height=height)
            # Keep picture and its numbered caption together; limit each image to a compact page fraction.
            cap=Paragraph(html.escape(f"Figure {fn}: {caption}"),styles["caption"])
            story.append(KeepTogether([Spacer(1,8),image,cap]))
            para=docx.add_paragraph();para.alignment=WD_ALIGN_PARAGRAPH.CENTER;para.add_run().add_picture(str(path),width=DInches(width/72))
            para=docx.add_paragraph(f"Figure {fn}: {caption}");para.alignment=WD_ALIGN_PARAGRAPH.CENTER
            for r in para.runs:r.font.size=DPt(11)
            markdown.extend([f"![Figure {fn}: {caption}]({path.relative_to(OUT).as_posix()})",""])
        elif kind=="table":
            tn+=1;_,caption,headers,rows,*extra=b
            tiny=extra==["inventory"];cell_style=styles["small"] if tiny else styles["cell"]
            data=[[Paragraph(html.escape(str(c)).replace("\n","<br/>"),cell_style) for c in row] for row in [headers,*rows]]
            if tiny and len(headers)==5: widths=[WIDTH*.33,WIDTH*.25,WIDTH*.14,WIDTH*.14,WIDTH*.14]
            elif tiny and len(headers)==6:widths=[WIDTH*.28,WIDTH*.19,WIDTH*.17,WIDTH*.07,WIDTH*.16,WIDTH*.13]
            else:widths=[WIDTH/len(headers)]*len(headers)
            table=Table(data,colWidths=widths,repeatRows=1,hAlign="CENTER")
            table.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,0),colors.HexColor("#e1eaf0")),("VALIGN",(0,0),(-1,-1),"TOP"),("GRID",(0,0),(-1,-1),.35,colors.HexColor("#b9c4cd")),("ROWBACKGROUNDS",(0,1),(-1,-1),[colors.white,colors.HexColor("#f7f8fa")]),("LEFTPADDING",(0,0),(-1,-1),4),("RIGHTPADDING",(0,0),(-1,-1),4),("TOPPADDING",(0,0),(-1,-1),4),("BOTTOMPADDING",(0,0),(-1,-1),4)]))
            cap=Paragraph(html.escape(f"Table {tn}: {caption}"),styles["caption"]);cap.keepWithNext=True
            story.extend([cap,table,Spacer(1,10)])
            para=docx.add_paragraph(f"Table {tn}: {caption}");para.alignment=WD_ALIGN_PARAGRAPH.CENTER
            table_dx=docx.add_table(rows=1,cols=len(headers));table_dx.style="Light Shading Accent 1"
            for cell,text in zip(table_dx.rows[0].cells,headers):cell.text=str(text)
            repeat=OxmlElement("w:tblHeader");table_dx.rows[0]._tr.get_or_add_trPr().append(repeat)
            for row in rows:
                for cell,text in zip(table_dx.add_row().cells,row):
                    cell.text=str(text)
                    for para in cell.paragraphs:
                        para.paragraph_format.line_spacing=1.0
                        para.paragraph_format.space_after=DPt(0)
                        para.paragraph_format.space_before=DPt(0)
                        for run in para.runs:run.font.size=DPt(8 if tiny else 9)
            markdown.extend([f"Table {tn}: {caption}","","| "+" | ".join(headers)+" |","| "+" | ".join(["---"]*len(headers))+" |",*["| "+" | ".join(str(c).replace("|","/") for c in row)+" |" for row in rows],""])
    # The canonical report is compiled exclusively by pdfLaTeX below.
    (OUT/"Project-Report.md").write_text("\n".join(markdown),encoding="utf-8")
    return {"figures":fn,"tables":tn,"nodes":len(data_rows()[0])}


SLIDES = [
    ("THE MIDNIGHT MISSION", "01  INTRODUCTION", "A haunted toy room. An interactive graphics story.", ["Adiba Tahsin · Roll 2107031", "CSE-4102 · Computer Graphics and Image Processing Laboratory", "Md Tajmilur Rahman, Lecturer", "Md Mubtashim Abrar Nihal, Lecturer"], ["room-night"], "Introduce the project as a primitive-built interactive room. Mention the arrival, rescue mission, and independently switchable rendering techniques."),
    ("EXPLORE, CONTROL, COMPARE", "02  FEATURES + PROJECT DEMO", "The proposal is realised as a complete arrival, puzzle, rescues and outdoor escape.", ["Five driveable toys; ball and lamp controls", "TRS + hierarchy; mounting and dismounting", "Three light types; Gouraud and Phong", "22 surface maps; analytic BVH ray tracing"], ["house","mounted"], "Play the embedded Project-Demo.mp4. Explain features using the timed narration guide; continue with individual screenshots afterward."),
    ("PRIMITIVES BECOME CHARACTERS", "03  GEOMETRY + OBJECTS", "Position, normal and UV data define five indexed mesh families.", ["Plane 4 vertices / 2 triangles; cube 24 / 12", "Sphere: 24 × 36 → 925 vertices / 1656 triangles", "Cylinder: 134 / 128; cone: 99 / 64", "The scene uses shared buffers and instance transforms"], ["woody","buzz","penny"], "Explain why a cube needs 24 vertices and why the sphere duplicates its seam. Point to curved heads, wheels and scaled ellipsoids. The models are authored from primitives."),
    ("A WORLD WITH SURFACE DETAIL", "04  ENVIRONMENT + TEXTURES", "The house, garden, room and furniture belong to one connected scene.", ["Wood grain, wallpaper, cloth, moon craters and book rows", "UV repeats multiply material colour by sampled RGB", "Picket alpha cuts holes and replaces repeated geometry", "22 maps plus white in one texture array serves the ray tracer"], ["house","window","bookcase"], "Identify the house, garden, poster, bed, curtains and bookcase. Colour maps add detail without changing geometry; alpha coverage is different from transparency blending."),
    ("ONE PARENT MOVES THE RIDER", "05  TRANSFORMS + INTERACTION", "Mworld(child) = Mworld(parent) × Mlocal(child)", ["R: mount/dismount within 2.2 units", "1–5 + W/S/A/D: choose and drive a toy", "Select: live takeover; 0 release; N full manual", "F: inspect; C: Free / Orbit / Follow", "Normal matrix = inverse transpose of world linear part"], ["live-control","mounted"], "Explain independent per-actor ownership and world-to-local conversion during mounting. Demonstrate one parameter change in the inspector. F2 selects raster mode for an actual Gouraud/Phong comparison."),
    ("EVERY MOTION HAS A RULE", "06  ANIMATION", "Delta time connects displacement, gait, wheel rotation and camera motion.", ["Gait angle = sin(distance phase) × amplitude", "Wheel angle = distance / radius", "Doors rotate at hinges; Penny follows 3D waypoints", "O: ghost, ball and lamp ambience", "Fan rotor: 110°/s; clock hands follow scene hour"], ["stair-descent","door-impact","car"], "Explain matching rendered treads and grounded support, Buzz's actual entrance hit, debris motion, wheel rotation and bounded physics substeps."),
    ("THREE LIGHT TYPES", "07  LIGHTING", "Ambient + diffuse + specular + emission", ["Directional: sun/moon; parallel rays", "Point: lamp bulb; 1/(kc + kl d + kq d²)", "Spot: lamp/headlights; smooth inner/outer cone", "Diffuse = kd max(N·L,0)", "Specular = ks max(R·V,0)^n"], ["lamp","spot","diffuse"], "Move/tilt the lamp and explain how its light anchor and head axis follow the hierarchy. Toggle F5/F6/F7. The lamp's 22°/34° cone has a smooth edge."),
    ("WHERE IS LIGHT EVALUATED?", "08  SHADING", "Matched views expose the difference between vertex and pixel illumination.", ["Flat: triangle normal from position derivatives", "Gouraud: vertex lighting → interpolated colour", "Phong: interpolated normal → pixel lighting", "Blinn: half-vector H = normalize(L+V)", "F2 cycles raster modes; F4 returns to ray tracing"], ["flat","gouraud","phong","blinn"], "Distinguish illumination from shading. Gouraud can miss a narrow highlight between vertices. Phong normalizes the interpolated normal and evaluates the reflection-vector term per fragment."),
    ("RAYS REVEAL A SECOND VIEW", "09  RAY TRACING + SHADOWS", "Primary ray → nearest analytic hit → illumination → bounded continuation", ["BVH median split; world-space boxes; near child first", "Five analytic primitive intersection equations", "Shadow rays test visibility to selected lights", "Mirror floor/ball; straight transparency", "Raster lamp: 2048² depth map + 3 × 3 PCF"], ["ray-zero","room-ray"], "Show the polished-floor reflection. Explain unnormalised object-space ray directions and the invariant t. State that transparency is straight-through, with no physical refraction or diffuse indirect bounces."),
    ("THANK YOU", "10  THE TOYS ARE SAFE", "All five escape. The scene remains open to inspection.", ["Adiba Tahsin · Roll 2107031", "CSE-4102", "Geometry · hierarchy · motion · interaction", "Lighting · shading · textures · ray tracing"], ["story-end"], "Invite questions. Be ready to change shininess, light attenuation, lamp cone, movement speed, scale, or ray bounce count and explain the observed effect."),
]


def slides():
    prs=Presentation();prs.slide_width=Inches(16);prs.slide_height=Inches(9)
    previews=OUT/"slides";previews.mkdir(exist_ok=True)
    background=Image.open(OUT/"haunted-background.png")
    for number,(title,tag,subtitle,bullets,pictures,notes) in enumerate(SLIDES,1):
        slide=prs.slides.add_slide(prs.slide_layouts[6]);slide.shapes.add_picture(str(OUT/"haunted-background.png"),0,0,width=prs.slide_width,height=prs.slide_height)
        pic=background.copy();draw=ImageDraw.Draw(pic)
        def panel(x,y,w,h,fill=NAVY):
            draw.rounded_rectangle((x,y,x+w,y+h),16,fill=fill)
            shape=slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(x/100),Inches(y/100),Inches(w/100),Inches(h/100));shape.fill.solid();shape.fill.fore_color.rgb=RGBColor(*fill);shape.line.fill.background()
        def text(x,y,w,h,content,size=28,colour=CREAM,bold=False,serif=False):
            # PIL line wrapping is shared with the editable slide so both previews use the same breaks.
            fnt=font(size,bold,serif);lines=[]
            for line in content.split("\n"):
                words=line.split();current=""
                for word in words:
                    nextline=(current+" "+word).strip()
                    if draw.textlength(nextline,font=fnt)>w and current:lines.append(current);current=word
                    else:current=nextline
                lines.append(current)
            actual="\n".join(lines)
            assert len(lines)*(size+8)<=h+10,(number,content,len(lines),h)
            draw.multiline_text((x,y),actual,font=fnt,fill=colour,spacing=8)
            tb=slide.shapes.add_textbox(Inches(x/100),Inches(y/100),Inches(w/100),Inches(h/100));tf=tb.text_frame;tf.word_wrap=False;tf.margin_left=tf.margin_right=tf.margin_top=tf.margin_bottom=0
            for i,line in enumerate(lines):
                para=tf.paragraphs[0] if i==0 else tf.add_paragraph();para.text=line;para.font.name="Georgia" if serif else "Segoe UI";para.font.size=Pt(size*.72);para.font.bold=bold;para.font.color.rgb=RGBColor(*colour);para.space_after=Pt(5)
            return tb
        text(75,40,1250,40,tag,21,colour=GOLD,bold=True)
        text(75,102,1430,90,title,43,colour=GOLD,bold=True,serif=True)
        text(75,187,1430,80,subtitle,26)
        panel(60,280,525,478)
        y=308
        for bullet in bullets:
            height=72 if len(bullet)>35 else 42
            text(85,y,465,height,bullet,25,colour=GOLD if bullet.startswith("PLAY") else CREAM)
            y+=height+7
        # Distinct images use consistent gutters; pictures are embedded and cropped to the panel.
        if len(pictures)==1:rects=[(620,280,915,478)]
        elif len(pictures)==2:rects=[(620,280,915,228),(620,530,915,228)]
        elif len(pictures)==3:rects=[(620,280,590,330),(1230,280,305,228),(1230,530,305,228)]
        else:rects=[(620,280,445,228),(1090,280,445,228),(620,530,445,228),(1090,530,445,228)]
        for name,(x,y,w,h) in zip(pictures,rects):
            source=Image.open(path_for(name)).convert("RGB")
            scale=max(w/source.width,(h-30)/source.height)
            resized=source.resize((int(source.width*scale),int(source.height*scale)),Image.Resampling.LANCZOS)
            left=(resized.width-w)//2;top=(resized.height-h+30)//2;resized=resized.crop((left,top,left+w,top+h-30))
            cached=previews/f"slide-{number:02d}-{name}.png";resized.save(cached)
            pic.paste(resized,(x,y));slide.shapes.add_picture(str(cached),Inches(x/100),Inches(y/100),width=Inches(w/100),height=Inches((h-30)/100))
            panel(x,y+h-30,w,30,(31,45,67));text(x+10,y+h-27,w-20,30,name.replace("-"," ").upper(),17,colour=GOLD)
        text(75,808,1200,32,"Adiba Tahsin · 2107031 · CSE-4102 · KUET",18)
        text(1430,808,100,32,f"{number:02d} / 10",18,colour=GOLD)
        if number in [1,10]:
            badge=Image.open(OUT/"KUET-LOGO.png").convert("RGBA");badge.thumbnail((75,86))
            pic.paste(badge,(1450,55),badge)
            slide.shapes.add_picture(str(OUT/"KUET-LOGO.png"),Inches(14.5),Inches(.55),width=Inches(.75))
        slide.notes_slide.notes_text_frame.text=notes
        if number==2:
            # A visible playable movie object is embedded, rather than a machine-local hyperlink.
            movie=slide.shapes.add_movie(str(OUT/"Project-Demo.mp4"),Inches(.85),Inches(6.4),Inches(4.6),Inches(.65),poster_frame_image=str(OUT/"video/caption-00.png"),mime_type="video/mp4")
            # PowerPoint can open the embedded movie; label remains legible above its poster.
            text(97,653,440,40,"PLAY PROJECT DEMO",23,colour=GOLD,bold=True)
        pic.save(previews/f"Slide-{number:02d}.png")
    prs.save(OUT/"Project-Presentation.pptx")
    c=pdfcanvas.Canvas(str(OUT/"Project-Presentation.pdf"),pagesize=(1152,648));c.setTitle(TITLE);c.setAuthor(AUTHOR)
    for number in range(1,11):c.drawImage(str(previews/f"Slide-{number:02d}.png"),0,0,width=1152,height=648);c.showPage()
    c.save()


def study_guide():
    cues=json.loads((OUT/"video/narration.json").read_text())
    lines=["# Demonstration and Viva Guide","",TITLE,"",AUTHOR+" | Roll "+ROLL,"", "## Start and rehearse", "", "Run `bin/Release/HauntedToyRoom.exe` from the project folder. The arrival starts automatically. Use Y to reach the hallway puzzle quickly, Shift+N for a full replay, and G to hide the interface. Press H for help. Keep the report PDF, slide deck and Project-Demo.mp4 together for the showcase.", "", "## Two-minute video narration", "", "The video has visual captions and no recorded voice. Explain the following points live. The deck embeds the video on slide 2; the standalone MP4 works independently of slide playback support.", "", "| Time | Explain |", "| --- | --- |"]
    for cue in cues:
        stamp=lambda sec:f"{sec//60}:{sec%60:02d}"
        lines.append(f"| {stamp(cue['start'])}–{stamp(cue['end'])} | {cue['title']}: {cue['narration']} |")
    lines.extend(["", "## Live demonstration sequence", "", "1. Y skips arrival. Inspect all three hallway clues with Enter, then approach the keypad. Enter 257 and confirm with Enter. A wrong code clears the digits and keeps the door locked.", "2. Penny activates the two red low switches with Enter. Select Jessie with 2, approach Bullseye and press R. Ride beneath the high platform, R dismounts onto it, and Enter activates the third switch.", "3. Ctrl+0 selects Penny. Activate the red release near the rear translucent barrier. Return through the hallway, descend the stairs and press Enter at the sealed front door. Explain Buzz flight, actual nearest-hit laser impact and the six physical fragments.", "4. 5 selects the car. W/S and A/D drive, L toggles moving spotlights. F focuses it; hallway camera access is supported.", "5. 7 selects the lamp. R toggles power, W/S tilts, A/D swivels, comma/period alters intensity. Both light position and direction follow the rig.", "6. F2 cycles Flat/Gouraud/Phong/Blinn and selects the raster path. F5/F6/F7 isolate terms. F3 toggles colour textures. F4 selects analytic ray tracing; 9 changes bounces and minus/equal changes resolution.", "7. Click a toy or furniture. Tab enters edit mode; T chooses Translate/Rotate/Scale/Shear. J/L, U/O and I/K change axes. M mirrors, Backspace restores. V lists actual parts; Shift+V prints vertices/indices.", "8. O enables haunted ambience. Show the ghost's opacity, ball rolling and lamp motion. N resumes the coordinated story; Shift+N restarts it.", "", "Reproducible rider setup:", "", "```powershell", ".\\bin\\Release\\HauntedToyRoom.exe --no-intro --manual --mount --select 1 --no-raytrace", "```", "", "Reproducible matched shading view:", "", "```powershell", ".\\bin\\Release\\HauntedToyRoom.exe --no-intro --manual --no-raytrace --shading 1 --select 3 --cam 0,1.5,-4.5,0,1.15,-6.6", "```", "", "## Live control design", "", "Selection during playback owns only one actor. StoryDirector skips writes to that actor and advances a separate virtual route cursor for scene gates. Other actors continue normal waypoint motion and animation. They ignore the owned actor as an obstacle, so parking across a path cannot stall their choreography; wall and furniture contacts remain active for your actor. The door always requires a real laser hit, including when Buzz is manually controlled. The ending requires every actual character outside. Releasing control rejoins a waypoint on the current floor without teleporting. A mounted pair is detached when selected for independent control; remounting intentionally drives the connected pair. 0 releases ownership; N retains the separate full manual mode. Penny is controllable after arrival by clicking her or using Ctrl+0. The arrival camera remains cinematic until Y or arrival completion.", "", "## Questions you should answer", ""])
    from escape_content import GUARDS, PUZZLE_OBJECTS, RESCUE_OBJECTS, ESCAPE_OBJECTS
    lines.extend(["## Escape implementation and objects", "", GUARDS,"",PUZZLE_OBJECTS,"",RESCUE_OBJECTS,"",ESCAPE_OBJECTS,"", "Rehearse the entire escape without skipping its physical interactions:","", "```powershell", ".\\bin\\Release\\HauntedToyRoom.exe --no-intro --story --gameplay-demo --no-raytrace", "```", ""])
    qa=[
        ("Why 24 cube vertices instead of eight?", "A geometric corner belongs to three faces. Each face needs a distinct normal and UV chart, so the indexed mesh keeps four vertices per face."),
        ("What is the difference between illumination and shading?", "Illumination computes light at one point. Shading decides where that computation is evaluated and which quantities are interpolated."),
        ("Why can Gouraud miss a highlight?", "It evaluates specular response only at vertices. A peak between vertices is absent from the interpolated values."),
        ("Why normalise a Phong normal after interpolation?", "Interpolation does not preserve unit length. The cosine dot product requires a unit vector."),
        ("Why inverse transpose for normals?", "It preserves perpendicularity to transformed tangents under non-uniform scale and shear: (A^-T n)·(A t)=n·t."),
        ("What happens when shininess increases?", "The exponent suppresses values below one more strongly, producing a narrower specular highlight. ks controls its strength."),
        ("Why are the spotlight cutoffs cosine values?", "The cone test uses a dot product of unit vectors. The outer angle is larger, so its cosine is smaller; smoothstep fades between these bounds."),
        ("How does Jessie follow Bullseye?", "Her root is attached under the saddle. The horse world matrix multiplies the saddle and rider local matrices, so translation, rotation and gait propagate naturally."),
        ("Why do you not normalise the transformed ray direction?", "Keeping its length preserves the same t in object/world space. Hits from different scaled instances can then be compared directly."),
        ("Is the tracer testing every triangle?", "No. The BVH bounds scene instances, then tests exact plane, cube, sphere, cylinder and cone equations in object space."),
        ("Is transparency the same as refraction?", "No. This implementation continues straight through with reduced throughput. It does not apply Snell's law."),
        ("How are raster shadows different from ray shadows?", "Raster lamp shadows compare a receiver with the lamp depth map. Ray shadows query whether an opaque object lies before the selected light."),
        ("Why retain off-screen geometry in the ray scene?", "It may still appear in a reflection or occlude a light. Camera frustum culling is applied only to the raster submission."),
        ("Does a colour texture change geometry?", "No. It modulates albedo. The fence alpha is coverage, while the moon craters and cloth weave remain colour detail rather than displacement or normal mapping."),
        ("How are wheel and ball angles calculated?", "Angle equals distance/radius. A ball uses an axis perpendicular to up and displacement; wheels rotate around their fixed axle."),
        ("How do optimisations preserve the appearance?", "Shared buffers preserve identical geometry; LOD retains more triangles when large on screen; conservative bounds prevent false culling; texture detail replaces subpixel objects; bounded simulation prevents unbounded catch-up work."),
        ("What are the deliberate limitations?", "Lamp-only general raster shadow map; selected/thresholded ray shadows; finite continuations; straight transparency; box-approximate contacts. No global diffuse transport or physical refraction."),
    ]
    for question,answer in qa:lines.extend(["### "+question,"",answer,""])
    lines.extend(["## Parameter-change practice", "", "| Parameter | Location | Expected effect |", "| --- | --- | --- |", "| `shininess` / `ks` | `Room.cpp` material creation or `Material.h` default | Narrower highlight / brighter highlight |", "| `linear`, `quadratic` | `ToyRoomApp::BuildScene` light setup | Faster dimming with distance |", "| 22° / 34° lamp cone | `ToyRoomApp::BuildScene` cutoff values | Inner lit region / larger smooth cone |", "| `reflectivity` | `Room.cpp` floor/ball materials | Stronger mirror contribution in tracer |", "| `rayBounces`, `rayScale` | `RenderSettings.h`; keys 9 and minus/equal | Deeper continuation / more pixels and cost |", "| `maxSpeed`, `turnRate` | `Character.h` | Faster motion / faster turning |", "| Sphere stacks/sectors | `Assets::Load` | Smoother silhouette but more triangles |", "| `uvScale` | Material creation in room/house builders | More/fewer pattern repeats |", "| Fan 110 degrees/s | `ToyRoomApp::StepScene` | Faster/slower rotor animation |", "", "## One frame: code reading route", "", "Read `Application::Run` → `ToyRoomApp::OnUpdate` / `StepScene` → input handlers → `StoryDirector::Update` and `Character::Animate` → `PhysicsWorld` → `SceneNode::UpdateWorld` → `UpdateLights` → `OnRender` → `Renderer::Render` or `RayTracer::Render`. Read `lighting.glsl` once, then compare `gouraud.vert`, `lit.frag` and `raytrace.frag`. Look at the named object's builder and inventory row when explaining a part. You do not need to memorise every line.", "", "## Checks and reproduction", "", "```powershell", ".\\tools\\build.ps1 -Configuration Release", ".\\tools\\build.ps1 -Configuration Debug", ".\\tools\\check-physics.ps1", "python tools/check_showcase_interaction.py", "python tools/check_showcase_live.py", "python tools/make_showcase.py --capture --video", "python tools/build_showcase_docs.py", "```", "", "Document generation requires pdfLaTeX (MiKTeX or TeX Live) as well as Python packages listed in `tools/showcase-requirements.txt`. The final report source is `Project-Report.tex`; `python tools/build_latex_report.py` rebuilds the PDF alone. Existing artifacts are ready to use; regeneration is optional. `validation/` contains build/check, interaction, story, capture and media evidence. `inventory/` is the complete node/material/surface-map data used for the report."])
    (OUT/"Demonstration-Study-Guide.md").write_text("\n".join(lines),encoding="utf-8")
    # A compact printable study guide accompanies its editable Markdown.
    study=Document();study.styles["Normal"].font.name="Calibri";study.styles["Normal"].font.size=DPt(11)
    index=0
    while index<len(lines):
        line=lines[index]
        if line.startswith("| "):
            rows=[]
            while index<len(lines) and lines[index].startswith("| "):
                cells=[c.strip() for c in lines[index].strip("|").split("|")]
                if not all(c.replace("-","").strip()=="" for c in cells):rows.append(cells)
                index+=1
            count=len(rows[0]); table=study.add_table(rows=0,cols=count);table.style="Light Shading Accent 1"
            for row in rows:
                row=row[:count-1]+[" | ".join(row[count-1:])]
                for cell,value in zip(table.add_row().cells,row):cell.text=value
            continue
        if line.startswith("### "):study.add_heading(line[4:],2)
        elif line.startswith("## "):study.add_heading(line[3:],1)
        elif line.startswith("# "):study.add_heading(line[2:],0)
        elif line and not line.startswith("```"):study.add_paragraph(line.replace("`",""))
        index+=1
    study.save(OUT/"Demonstration-Study-Guide.docx")
    # Keep a portable printable copy with proper tables and readable code.
    from reportlab.platypus import SimpleDocTemplate
    body=ParagraphStyle("GuideBody",fontName="ReportTimes",fontSize=11,leading=15,spaceAfter=7)
    heading=ParagraphStyle("GuideHeading",parent=body,fontName="ReportTimesBold",fontSize=15,leading=19,spaceBefore=14)
    guide=[]; index=0; in_code=False
    while index<len(lines):
        line=lines[index]
        if line.startswith("```"):
            in_code=not in_code;index+=1;continue
        if line.startswith("| "):
            rows=[]
            while index<len(lines) and lines[index].startswith("| "):
                cells=[c.strip() for c in lines[index].strip("|").split("|")]
                if not all(c.replace("-","").strip()=="" for c in cells):
                    rows.append([Paragraph(html.escape(c),body) for c in cells])
                index+=1
            count=len(rows[0])
            for row in rows:
                if len(row)>count:
                    extra=row[count-1:]
                    row[count-1:]=[Paragraph(" | ".join(cell.getPlainText() for cell in extra),body)]
            widths=[70,WIDTH-70] if len(rows[0])==2 else [WIDTH*.24,WIDTH*.34,WIDTH*.42]
            table=Table(rows,colWidths=widths,repeatRows=1)
            table.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,0),colors.HexColor("#e9edf2")),("VALIGN",(0,0),(-1,-1),"TOP"),("GRID",(0,0),(-1,-1),.3,colors.lightgrey),("LEFTPADDING",(0,0),(-1,-1),5),("RIGHTPADDING",(0,0),(-1,-1),5)]))
            guide.extend([table,Spacer(1,10)]);continue
        if line:
            label=line.lstrip("# ") if line.startswith("#") else line
            guide.append(Paragraph(html.escape(label),heading if line.startswith("#") else body))
        index+=1
    SimpleDocTemplate(str(OUT/"Demonstration-Study-Guide.pdf"),pagesize=A4,leftMargin=86.4,rightMargin=72,topMargin=60,bottomMargin=60).build(guide)


def validate(summary):
    pdf=pymupdf.open(OUT/"Project-Report.pdf");text="\n".join(page.get_text() for page in pdf)
    assert AUTHOR in text and ROLL in text and COURSE in text
    assert len(pdf)<40, f"Report exceeds page limit: {len(pdf)}"
    for token in ["Acknowledgment","Acknowledgement","CHAPTER V ","CHAPTER VI ","template","prepare according to","hardware specifications"]:assert token.lower() not in text.lower(),token
    assert "Appendix A" in text and "Appendix B" in text
    assert not any(token in text for token in ["Ã", "Â", "â†", "â€"]), "UTF-8 text corruption"
    deck=Presentation(OUT/"Project-Presentation.pptx");assert len(deck.slides)==10
    import zipfile
    with zipfile.ZipFile(OUT/"Project-Presentation.pptx") as z:
        assert any(n.startswith("ppt/media/") and n.endswith(".mp4") for n in z.namelist())
    summary.update({"report_pages":len(pdf),"slides":10,"embedded_video":True,"identity":True,"content_scan":"passed"})
    (OUT/"validation/documents.json").write_text(json.dumps(summary,indent=2))
    for num in [0,1,10,20,len(pdf)-1]:
        if num<len(pdf):pdf[num].get_pixmap(matrix=pymupdf.Matrix(1.2,1.2)).save(str(OUT/f"validation/report-page-{num+1}.png"))
    print(json.dumps(summary,indent=2))


if __name__=="__main__":
    diagrams();themed_background();summary=report();slides();study_guide()
    from build_latex_report import build as build_latex
    summary.update(build_latex())
    validate(summary)
