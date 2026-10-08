"""Generate and compile the canonical LaTeX report; fail above 39 pages."""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import pymupdf
from compact_report import compact_blocks
from latex_equations import EQUATIONS
import build_showcase_docs as source
from showcase_content import AUTHOR, ROLL, COURSE, COURSE_NAME, TITLE, TEACHERS, TEXTURES

OUT=source.OUT


def escape(text):
    replacements={"\\":r"\textbackslash{}", "&":r"\&", "%":r"\%", "$":r"\$", "#":r"\#",
                  "_":r"\_", "{":r"\{", "}":r"\}", "~":r"\textasciitilde{}", "^":r"\textasciicircum{}",
                  "\u00d7":r"\(\times\)", "\u00b7":r"\(\cdot\)", "\u03a3":r"\(\sum\)",
                  "\u2264":r"\(\le\)","\u2265":r"\(\ge\)","\u2212":"-","\u00b2":r"\(^{2}\)",
                  "\u00b0":r"\(^{\circ}\)","\u00bd":r"\(\tfrac12\)","\u03c0":r"\(\pi\)",
                  "\u2014":"---","\u2013":"--","\u201c":"``","\u201d":"''","\u2019":"'",
                  "\u2192":r"\(\to\)","\u03b5":r"\(\varepsilon\)"}
    # URL line breaking is managed by hyperref instead of escaping it as prose.
    chunks=re.split(r"(https?://\S+)",str(text))
    result=[]
    for chunk in chunks:
        if chunk.startswith(('https://','http://')):result.append(r'\url{'+chunk+'}')
        else:result.append(''.join(replacements.get(c,c) for c in chunk))
    return ''.join(result)


PREAMBLE=r"""\documentclass[12pt,a4paper]{article}
\usepackage[T1]{fontenc}
\usepackage[utf8]{inputenc}
\usepackage{mathptmx,amsmath,amssymb,graphicx}
\usepackage[a4paper,left=27mm,right=23mm,top=23mm,bottom=23mm]{geometry}
\usepackage{booktabs,longtable,array,caption,xcolor,fancyhdr,hyperref,float}
\definecolor{navy}{RGB}{15,25,47}
\hypersetup{colorlinks=true,linkcolor=navy,urlcolor=navy,pdftitle={Haunted Toy Room: The Midnight Mission},pdfauthor={Adiba Tahsin}}
\setlength{\parindent}{0pt}
\setlength{\parskip}{5pt plus 1pt minus 1pt}
\linespread{1.10}
\setlength{\emergencystretch}{2em}
\setlength{\headheight}{14pt}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{\small Haunted Toy Room: The Midnight Mission}
\fancyhead[R]{\small CSE-4102}
\fancyfoot[C]{\thepage}
\renewcommand{\headrulewidth}{0.3pt}
\captionsetup{font=small,labelfont=bf,skip=5pt}
\setlength{\textfloatsep}{10pt plus 2pt minus 2pt}
\setlength{\floatsep}{8pt plus 2pt minus 2pt}
\setlength{\intextsep}{8pt plus 2pt minus 2pt}
\renewcommand{\topfraction}{0.9}
\renewcommand{\bottomfraction}{0.8}
\renewcommand{\textfraction}{0.08}
\renewcommand{\floatpagefraction}{0.75}
\setcounter{topnumber}{3}
\setcounter{bottomnumber}{3}
\setcounter{totalnumber}{5}
\setcounter{tocdepth}{2}
\begin{document}
\begin{titlepage}
\centering
\includegraphics[width=24mm]{KUET-LOGO.png}\par\vspace{8mm}
{\large\bfseries CSE-4102\par}
{\large Computer Graphics and Image Processing Laboratory\par}\vspace{9mm}
{\LARGE\bfseries HAUNTED TOY ROOM\par}
{\Large\bfseries THE MIDNIGHT MISSION\par}\vspace{6mm}
{\large Project Report\par}\vspace{9mm}
{\large\bfseries Adiba Tahsin\par}
{\large Roll: 2107031\par}\vspace{9mm}
{\large Course Teachers\par}\vspace{3mm}
Md Tajmilur Rahman, Lecturer\par
Md Mubtashim Abrar Nihal, Lecturer\par\vspace{10mm}
Department of Computer Science and Engineering\par
Khulna University of Engineering \& Technology\par
Khulna 9203, Bangladesh\par\vspace{6mm}
October 2026
\end{titlepage}
\setcounter{page}{2}
"""


def table(caption, headers, rows, number):
    n=len(headers)
    if caption.startswith('Surface-map'):fractions=[.23,.44,.33]
    elif caption.startswith('Complete scene'):fractions=[.29,.18,.53]
    elif n==3:fractions=[.23,.40,.37]
    elif n==4:fractions=[.19,.28,.20,.33]
    elif n==5:fractions=[.26,.23,.10,.20,.21]
    else:fractions=[1/n]*n
    spec=''.join(r'>{\raggedright\arraybackslash}p{\dimexpr'+f'{f:.4f}'+r'\linewidth-2\tabcolsep\relax}' for f in fractions)
    heading=' & '.join(r'\textbf{'+escape(c)+'}' for c in headers)+r' \\\midrule'
    lines=[r'{\small\setlength{\tabcolsep}{4pt}\renewcommand{\arraystretch}{1.12}',
           r'\begin{longtable}{'+spec+'}',r'\caption{'+escape(caption)+r'}\label{tab:'+str(number)+r'}\\',
           r'\toprule',heading,r'\endfirsthead',r'\multicolumn{'+str(n)+r'}{l}{\small\itshape Table \thetable{} (continued)}\\',r'\toprule',heading,r'\endhead',r'\bottomrule\endfoot']
    for row in rows:lines.append(' & '.join(escape(c) for c in row)+r' \\')
    lines.extend([r'\end{longtable}',r'}'])
    return '\n'.join(lines)


def build():
    source.diagrams()
    objects,materials,textures=source.data_rows()
    blocks=compact_blocks(source.expanded_blocks(),objects,materials,textures,TEXTURES,OUT,source.path_for)
    tex=[PREAMBLE];equation=figures=tables=0;contents=False
    for b in blocks:
        kind=b[0]
        if kind=='heading':
            _,title,level=b
            if title.startswith('CHAPTER I ') and not contents:
                tex.extend([r'\clearpage',r'\tableofcontents',r'\clearpage']);contents=True
            if level<=1:
                if title.startswith('CHAPTER ') and not title.startswith('CHAPTER I '):tex.append(r'\clearpage')
                tex.append(r'\section*{'+escape(title)+'}')
                tex.append(r'\addcontentsline{toc}{section}{'+escape(title)+'}')
            else:
                tex.append(r'\subsection*{'+escape(title)+'}')
                tex.append(r'\addcontentsline{toc}{subsection}{'+escape(title)+'}')
        elif kind=='paragraph':tex.extend([escape(b[1]),''])
        elif kind=='equation':
            assert equation<len(EQUATIONS)
            tex.extend([r'{\small\begin{equation}\begin{gathered}',EQUATIONS[equation],r'\end{gathered}\label{eq:'+str(equation+1)+r'}\end{equation}}'])
            equation+=1
        elif kind=='figure':
            _,name,caption=b;path=source.path_for(name).relative_to(OUT).as_posix();figures+=1
            limit='0.39' if name in ['room-furniture','animated-props','light-comparison','texture-atlas'] else ('0.22' if name in ['woody','jessie','buzz','car','penny','lamp','laser','window','blocks','house','scene-overview'] else '0.30')
            tex.extend([r'\begin{figure}[H]',r'\centering',r'\includegraphics[width=\linewidth,height='+limit+r'\textheight,keepaspectratio]{'+path+'}',
                        r'\caption{'+escape(caption)+r'}\label{fig:'+name+'}',r'\end{figure}'])
        elif kind=='table':tables+=1;tex.append(table(b[1],b[2],b[3],tables))
    assert equation==len(EQUATIONS),(equation,len(EQUATIONS))
    tex.append(r'\end{document}')
    (OUT/'Project-Report.tex').write_text('\n'.join(tex),encoding='utf-8')
    engine=shutil.which('pdflatex')
    assert engine,'pdflatex is required (MiKTeX or TeX Live)'
    env=dict(os.environ)
    env['PATH']=os.pathsep.join(p for p in env.get('PATH','').split(os.pathsep) if Path(p).is_dir())
    for run in range(3):
        proc=subprocess.run([engine,'--enable-installer','-interaction=nonstopmode','-halt-on-error','Project-Report.tex'],cwd=OUT,env=env,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=240)
        (OUT/f'validation/latex-pass-{run+1}.log').write_text('\n'.join(line.rstrip() for line in (proc.stdout+proc.stderr).splitlines())+'\n',encoding='utf-8')
        if proc.returncode:raise RuntimeError((proc.stdout+proc.stderr)[-4500:])
    doc=pymupdf.open(OUT/'Project-Report.pdf')
    assert len(doc)<40,f'Page limit exceeded: {len(doc)}'
    body='\n'.join(p.get_text() for p in doc)
    assert all(s in body for s in [AUTHOR,ROLL,COURSE,'Gouraud','Phong','Appendix A','Appendix B'])
    for banned in ['template','acknowledgement','hardware specifications','CHAPTER V ','CHAPTER VI ']:assert banned.lower() not in body.lower()
    chapter_starts=[]
    for number,page in enumerate(doc,1):
        for block in page.get_text('dict')['blocks']:
            for line in block.get('lines',[]):
                for span in line['spans']:
                    if span['text'].startswith('CHAPTER ') and span['size']>16:
                        assert span['bbox'][1]<90, 'Chapter does not begin a new page'
                        chapter_starts.append(number)
    assert len(chapter_starts)==4,chapter_starts
    contents_pages=[i+1 for i,p in enumerate(doc) if any(s['text']=='Contents' and s['size']>16 for b in p.get_text('dict')['blocks'] for l in b.get('lines',[]) for s in l['spans'])]
    assert len(contents_pages)==1 and contents_pages[0]>2
    toc='\n'.join(doc[i].get_text() for i in range(contents_pages[0]-1,chapter_starts[0]-1))
    sections=[b[1].split()[0] for b in blocks if b[0]=='heading' and b[2]>1]
    assert all(re.search(r'\b'+re.escape(section)+r'\s',toc) for section in sections), 'Incomplete contents entries'
    log=(OUT/'Project-Report.log').read_text(encoding='utf8',errors='replace')
    assert 'Overfull' not in log, 'Content extends beyond the layout'
    assert 'Missing character:' not in log, 'Missing font glyph'
    assert 'undefined references' not in log, 'Unresolved references'
    summary={'report_pages':len(doc),'figures':figures,'tables':tables,'equation_groups':equation,'scene_nodes':len(objects),'engine':'pdfLaTeX','page_limit':39,'content_scan':'passed','overfull_boxes':len(re.findall('Overfull',log)),'chapter_start_pages':chapter_starts,'contents_start_page':contents_pages[0],'contents_includes_sections':True,'contents_section_entries':len(sections)}
    (OUT/'validation/latex-report.json').write_text(json.dumps(summary,indent=2),encoding='utf8')
    for number,page in enumerate(doc,1):page.get_pixmap(matrix=pymupdf.Matrix(.9,.9)).save(str(OUT/f'validation/latex-page-{number:02d}.png'))
    print(json.dumps(summary,indent=2))
    return summary


if __name__=='__main__':build()
