"""Mathematical displays aligned with the report's 28 equation blocks."""
EQUATIONS = [
r"""\mathbf v=(p_x,p_y,p_z,n_x,n_y,n_z,u,v),\qquad s_v=8\cdot4=32\text{ bytes}\\
T_k=(I_{3k},I_{3k+1},I_{3k+2})""",
r"""\mathbf p=\tfrac12(\cos\phi\sin\theta,\sin\phi,\cos\phi\cos\theta),\qquad \mathbf n=2\mathbf p\\
u=\frac{\theta}{2\pi},\qquad v=\frac{\phi}{\pi}+\frac12,\qquad
N_T=2N_{\rm sectors}(N_{\rm stacks}-1)""",
r"""\mathbf p_{\rm cyl}=(\tfrac12\sin\theta,y,\tfrac12\cos\theta),\quad
\mathbf n_{\rm cyl}=(\sin\theta,0,\cos\theta)\\
\mathbf n_{\rm cone}=\operatorname{normalize}(\sin\theta,\tfrac12,\cos\theta)\\
N_{V,\rm cyl}=4m+6,\quad N_{T,\rm cyl}=4m,\quad
N_{V,\rm cone}=3m+3,\quad N_{T,\rm cone}=2m""",
r"""M_{\rm local}=T(\mathbf p)R_y(\psi)R_x(\vartheta)R_z(\varphi)BS(\mathbf s)\\
T=\begin{bmatrix}1&0&0&t_x\\0&1&0&t_y\\0&0&1&t_z\\0&0&0&1\end{bmatrix},\quad
S=\operatorname{diag}(s_x,s_y,s_z,1)\\
R_y(a)=\begin{bmatrix}\cos a&0&\sin a&0\\0&1&0&0\\-\sin a&0&\cos a&0\\0&0&0&1\end{bmatrix}""",
r"""R(\mathbf a,\theta)=\cos\theta I+(1-\cos\theta)\mathbf a\mathbf a^T+\sin\theta[\mathbf a]_\times\\
M_{\rm pivot}=T(\mathbf q)RT(-\mathbf q),\qquad x'=x+ky,\qquad F=I-2\mathbf n\mathbf n^T""",
r"""M_{\rm world,c}=M_{\rm world,p}M_{\rm local,c},\qquad A=\operatorname{mat}_3(M_{\rm world})\\
\mathbf N=\operatorname{normalize}(A^{-T}\mathbf n),\qquad
(A^{-T}\mathbf n)\cdot(A\mathbf t)=\mathbf n\cdot\mathbf t=0""",
r"""\mathbf f=\operatorname{normalize}(\mathbf{target}-\mathbf{eye}),\quad
\mathbf r=\operatorname{normalize}(\mathbf f\times\mathbf{up}),\quad\mathbf u=\mathbf r\times\mathbf f\\
V=\begin{bmatrix}\mathbf r^T&-\mathbf r\cdot\mathbf e\\\mathbf u^T&-\mathbf u\cdot\mathbf e\\-\mathbf f^T&\mathbf f\cdot\mathbf e\\0\;0\;0&1\end{bmatrix},\qquad
\mathbf p_{\rm clip}=PVM\mathbf p,\quad \mathbf p_{\rm NDC}=\frac{\mathbf p_{\rm clip,xyz}}{p_{\rm clip,w}}\\
P=\begin{bmatrix}(a\tan(\omega/2))^{-1}&0&0&0\\0&\cot(\omega/2)&0&0\\0&0&-\frac{f+n}{f-n}&-\frac{2fn}{f-n}\\0&0&-1&0\end{bmatrix}""",
r"""\mathbf d=\operatorname{normalize}\!\left(\mathbf f+x_{\rm NDC}a\tan(\omega/2)\mathbf r+y_{\rm NDC}\tan(\omega/2)\mathbf u\right)\\
\mathbf o'=M^{-1}(\mathbf o,1),\qquad \mathbf d'=M^{-1}(\mathbf d,0)""",
r"""\mathbf C=\mathbf C_{\rm material}\odot\mathbf C_{\rm texture},\qquad
\mathbf R_i=2(\mathbf N\cdot\mathbf L_i)\mathbf N-\mathbf L_i\\
\mathbf I=\mathbf C\odot\left[k_a\mathbf I_a+\sum_i v_i a_i s_i\mathbf I_{l,i}k_d\max(\mathbf N\cdot\mathbf L_i,0)\right]\\
\hphantom{\mathbf I=}+\sum_i v_i a_i s_i\mathbf I_{l,i}k_s S_i+\mathbf E\\
S_i=\mathbf 1\{\mathbf N\cdot\mathbf L_i>0\}\max(\mathbf R_i\cdot\mathbf V,0)^n""",
r"""\mathbf L_{\rm dir}=\operatorname{normalize}(-\mathbf d_l),\quad a_{\rm dir}=1\\
\mathbf L_{\rm point}=\frac{\mathbf p_l-\mathbf P}{d},\qquad a_{\rm point}=\frac1{k_c+k_l d+k_qd^2}\\
\theta=(-\mathbf L)\cdot\mathbf a_l,\qquad
q=\operatorname{clamp}\!\left(\frac{\theta-\cos\theta_o}{\cos\theta_i-\cos\theta_o},0,1\right),\qquad s=q^2(3-2q)""",
r"""a_{\rm pixel}=\frac{\sum_{j=0}^2\lambda_j a_j/w_j}{\sum_{j=0}^2\lambda_j/w_j}\\
\mathbf C_G=\mathbf C\odot\operatorname{interp}(\mathbf I_a+\mathbf I_d)+\operatorname{interp}(\mathbf I_s)+\mathbf E\\
\mathbf N_P=\operatorname{normalize}(\operatorname{interp}(\mathbf n)),\quad
\mathbf H=\operatorname{normalize}(\mathbf L+\mathbf V),\quad S_B=\max(\mathbf N\cdot\mathbf H,0)^{4n}""",
r"""\mathbf{uv}_s=\mathbf{uv}\odot\mathbf{uv}_{\rm scale},\qquad\mathbf{uv}_{\rm repeat}=\operatorname{fract}(\mathbf{uv}_s)\\
\mathbf C_s=(1-a)(1-b)\mathbf C_{00}+a(1-b)\mathbf C_{10}+(1-a)b\mathbf C_{01}+ab\mathbf C_{11}\\
\mathbf C_{\rm tri}=(1-\eta)\mathbf C_{\rm mip,\ell}+\eta\mathbf C_{\rm mip,\ell+1}""",
r"""s_{\rm BMP}=4\left\lceil\frac{Wb}{32}\right\rceil,\qquad (B,G,R)\longmapsto(R,G,B,255)\\
h>0:\text{ bottom-up rows},\qquad h<0:\text{ top-down rows}""",
r"""\mathbf q=\tfrac12\frac{(L_{VP}(\mathbf P,1))_{xyz}}{(L_{VP}(\mathbf P,1))_w}+\tfrac12\\
\beta=\max\{0.0009(1-\max(\mathbf N\cdot\mathbf L,0)),0.00012\}\\
v=\frac19\sum_{j=1}^{9}\mathbf 1\{q_z-\beta\le D_j\}""",
r"""\mathbf P(t)=\mathbf o+t\mathbf d,\qquad t>\varepsilon\\
\text{Sphere: }(\mathbf d\cdot\mathbf d)t^2+2(\mathbf o\cdot\mathbf d)t+\mathbf o\cdot\mathbf o-\tfrac14=0\\
\text{Cylinder: }(d_x^2+d_z^2)t^2+2(o_xd_x+o_zd_z)t+o_x^2+o_z^2-\tfrac14=0\\
\text{Cone: }x^2+z^2=\tfrac14(\tfrac12-y)^2,\qquad-\tfrac12\le y\le\tfrac12\\
\text{Plane: }t=-o_y/d_y,\quad |x|,|z|\le\tfrac12;\quad
t_{\rm enter}=\max_a\min(t_{a0},t_{a1}),\quad t_{\rm exit}=\min_a\max(t_{a0},t_{a1})""",
r"""\mathbf d_{b+1}=\mathbf d_b-2(\mathbf d_b\cdot\mathbf N)\mathbf N,\qquad
\mathbf o_{b+1}=\mathbf P+0.002\mathbf N\\
\mathbf C_{\rm acc}\mathrel{+}=\boldsymbol\tau_b\odot(1-\rho_b)\mathbf C_{{\rm local},b},\qquad
\boldsymbol\tau_{b+1}=\rho_b\boldsymbol\tau_b\\
\text{Transmission: }\mathbf C_{\rm acc}\mathrel{+}=\boldsymbol\tau_b\odot\alpha_b\mathbf C_{{\rm local},b},\quad
\boldsymbol\tau_{b+1}=(1-\alpha_b)\boldsymbol\tau_b,\quad\mathbf o_{b+1}=\mathbf P+0.002\mathbf d_b""",
r"""\rho_0=0.2,\quad\rho_1=0.5,\quad
\mathbf C_0=(0.6,0.3,0.1),\quad\mathbf C_1=(0.2,0.4,0.8),\quad\mathbf C_2=(0.1,0.1,0.2)\\
\mathbf C_{\rm final}=0.8\mathbf C_0+0.1\mathbf C_1+0.1\mathbf C_2=(0.51,0.29,0.18)""",
r"""\mathbf c=M_{0:2,3},\quad\mathbf h=\tfrac12(|\mathbf a_0|+|\mathbf a_1|+|\mathbf a_2|)\\
B_{\rm node}=\operatorname{union}(B_{\rm left},B_{\rm right}),\quad
r=\tfrac12\max_{s_y,s_z\in\{-1,1\}}\|\mathbf a_0+s_y\mathbf a_1+s_z\mathbf a_2\|""",
r"""\mathbf f=(\sin\psi,0,\cos\psi),\quad
v_{t+1}=v_t+\operatorname{clamp}(v_*-v_t,-a\Delta t,a\Delta t)\\
\psi_{t+1}=\psi_t+u_{\rm turn}\omega\Delta t,\quad
\mathbf p_{t+1}=\mathbf p_t+\mathbf f v_{t+1}\Delta t\\
\varphi_{t+1}=\varphi_t+4v\Delta t,\quad\theta_{\rm leg}=35^\circ\sin\varphi\,b_{\rm move}""",
r"""M_{{\rm rider},\rm local}^{\rm new}=M_{{\rm saddle},\rm world}^{-1}M_{{\rm rider},\rm world}\\
M_{{\rm rider},\rm world}=M_{{\rm horse},\rm world}M_{{\rm saddle},\rm local}M_{{\rm rider},\rm local}\\
q=\operatorname{clamp}(t/T,0,1),\qquad w(q)=q^2(3-2q)""",
r"""\theta_{\rm wheel}\mathrel{+}=\frac{\Delta s}{r_{\rm wheel}},\quad
\mathbf p_l=M_{{\rm anchor},:,3},\quad\mathbf d_l=\operatorname{normalize}(\mathbf a_z+\mathbf d_{\rm tilt})""",
r"""\theta_{\rm stair}=\operatorname{atan2}(4.5,8)=29.36^\circ\\
n(z)=\operatorname{clamp}\left(\left\lfloor18(z+7)/8\right\rfloor+1,1,18\right),\quad y=-4.5+0.25n(z)\\
h=20.5+2.5\operatorname{smoothstep}(s/S)""",
r"""a=(h-6)\pi/12,\quad H_s=\sin a,\quad
\mathbf p_s=\mathbf c_s+(-3.2\cos a,3.1\sin a,0)\\
\mathbf p_m=\operatorname{orbit}(a+\pi),\qquad
D=\operatorname{smoothstep}(-0.1,0.25,H_s)""",
r"""\psi_{\rm fan}=(\psi_{\rm fan}+110^\circ\Delta t)\bmod360^\circ\\
\theta_h=-30^\circ(h\bmod12),\qquad\theta_m=-360^\circ\operatorname{fract}(h)""",
r"""\mathbf a=\operatorname{normalize}(\mathbf{up}\times\Delta\mathbf p),\quad\theta=\|\Delta\mathbf p\|/r,\quad B_{t+1}=R(\mathbf a,\theta)B_t\\
\mathbf p_g=(4.5\sin(0.25t),\,4.2+0.35\sin(1.3t),\,3\cos(0.25t)-0.5)""",
r"""\boldsymbol\delta=\mathbf q_{\rm next}-\mathbf p_v,\quad d=\|\boldsymbol\delta\|,\quad
\mathbf p_{v,t+1}=\mathbf p_{v,t}+\boldsymbol\delta\frac{\min(d,v_{\rm route}\Delta t)}{d}\quad(d>0)\\
G=G_{\rm door,broken}\land\bigwedge_i\{z_i>13\land y_i<-0.3\}""",
r"""v_{y,t+1}=v_{y,t}-9.81\Delta t,\quad\mathbf p_{t+1}=\mathbf p_t+\mathbf v\Delta t\\
B_{\rm expanded}=[\mathbf b_{\min}-\mathbf h,\mathbf b_{\max}+\mathbf h],\quad
\Delta\mathbf p_{\rm slide}=\Delta\mathbf p-\mathbf n\min(0,\Delta\mathbf p\cdot\mathbf n)\\
\Delta t_{\rm physics}=1/120\text{ s},\qquad N_{\rm substeps}\le6""",
r"""s=\frac{r}{\|\mathbf c-\mathbf e\|},\qquad
\mathrm{LOD}(s)=\begin{cases}\text{low},&s<0.012\\\text{medium},&0.012\le s<0.06\\\text{full},&s\ge0.06\end{cases}\\
(M^{-1})_{i,:}=(\mathbf n_i^T,-\mathbf n_i\cdot\mathbf t),\quad
\mathbf n_i=\text{column }i\text{ of the normal matrix}""",
]

EQUATIONS.insert(25,r"T=\exp(-\rho d),\qquad \mathbf C_{\rm fogged}=T\mathbf C_{\rm rendered}+(1-T)\mathbf C_{\rm fog}")
