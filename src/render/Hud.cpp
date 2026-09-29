#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Hud.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace {
const glm::vec4 Ink(0.055f, 0.068f, 0.08f, 0.93f);
const glm::vec4 Cream(0.95f, 0.92f, 0.84f, 1.0f);
const glm::vec4 Muted(0.68f, 0.73f, 0.75f, 1.0f);
const glm::vec4 Gold(0.89f, 0.70f, 0.38f, 1.0f);
constexpr float Margin = 24.0f;
const char* Toys[8] = {"Woody", "Jessie", "Bullseye", "Buzz", "RC Car", "Ball", "Lamp", "Ghost"};
}

Hud::Font Hud::MakeFont(const wchar_t* family, int pixels, int weight)
{
	constexpr int atlasWidth = 1024, atlasHeight = 512, cell = 64;
	HDC dc = CreateCompatibleDC(nullptr);
	if (!dc) throw std::runtime_error("Cannot create HUD font context");
	BITMAPINFO info{};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = atlasWidth; info.bmiHeader.biHeight = -atlasHeight;
	info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
	void* data = nullptr;
	HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &data, nullptr, 0);
	HFONT font = CreateFontW(-pixels, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, family);
	if (!bitmap || !font || !data) {
		if (bitmap) DeleteObject(bitmap); if (font) DeleteObject(font); DeleteDC(dc);
		throw std::runtime_error("Cannot create HUD font atlas");
	}
	HGDIOBJ oldBitmap = SelectObject(dc, bitmap), oldFont = SelectObject(dc, font);
	std::memset(data, 0, atlasWidth * atlasHeight * 4);
	SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255));
	Font result;
	TEXTMETRICW metrics{}; GetTextMetricsW(dc, &metrics); result.height = static_cast<float>(metrics.tmHeight);
	for (int index = 0; index < 95; ++index) {
		const wchar_t c = static_cast<wchar_t>(index + 32);
		SIZE size{}; GetTextExtentPoint32W(dc, &c, 1, &size);
		const int x = (index % 16) * cell + 4, y = (index / 16) * cell + 4;
		TextOutW(dc, x, y, &c, 1);
		result.glyphs[static_cast<size_t>(index)] = {{static_cast<float>(x) / atlasWidth, static_cast<float>(y) / atlasHeight,
			static_cast<float>(x + size.cx) / atlasWidth, static_cast<float>(y + metrics.tmHeight) / atlasHeight}, static_cast<float>(size.cx)};
	}
	GdiFlush();
	const auto* source = static_cast<unsigned char*>(data);
	std::vector<unsigned char> coverage(atlasWidth * atlasHeight);
	for (size_t i = 0; i < coverage.size(); ++i) coverage[i] = std::max({source[i * 4], source[i * 4 + 1], source[i * 4 + 2]});
	coverage[0] = coverage[1] = coverage[atlasWidth] = coverage[atlasWidth + 1] = 255;
	SelectObject(dc, oldFont); SelectObject(dc, oldBitmap); DeleteObject(font); DeleteObject(bitmap); DeleteDC(dc);
	glGenTextures(1, &result.texture); glBindTexture(GL_TEXTURE_2D, result.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, coverage.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return result;
}

void Hud::Init()
{
	shader = Shader("shaders/hud.vert", "shaders/hud.frag");
	body = MakeFont(L"Bahnschrift", 22, FW_NORMAL);
	title = MakeFont(L"Georgia", 36, FW_NORMAL);
	glGenVertexArrays(1, &vao); glGenBuffers(1, &buffer);
	glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, p)));
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
	glBindVertexArray(0);
}
Hud::~Hud()
{
	if (body.texture) glDeleteTextures(1, &body.texture);
	if (title.texture) glDeleteTextures(1, &title.texture);
	if (buffer) glDeleteBuffers(1, &buffer);
	if (vao) glDeleteVertexArrays(1, &vao);
}
float Hud::Scale(int width, int height)
{
	return std::clamp(std::min(static_cast<float>(width) / 1600.0f, static_cast<float>(height) / 900.0f), 1.0f, 1.25f);
}
void Hud::Rect(float x, float y, float w, float h, glm::vec4 c)
{
	const glm::vec2 uv(0.0005f, 0.001f);
	for (const glm::vec2 p : {glm::vec2(x, y), glm::vec2(x + w, y), glm::vec2(x + w, y + h),
		glm::vec2(x, y), glm::vec2(x + w, y + h), glm::vec2(x, y + h)}) bodyVertices.push_back({p, uv, c});
}
void Hud::Text(const std::string& content, float x, float y, float size, glm::vec4 color, float maxWidth, bool heading)
{
	const Font& font = heading ? title : body;
	auto& vertices = heading ? titleVertices : bodyVertices;
	const float start = x, line = font.height * size * 1.25f;
	std::istringstream words(content); std::string word;
	while (words >> word) {
		float wordWidth = 0;
		for (unsigned char c : word) wordWidth += font.glyphs[static_cast<size_t>((c >= 32 && c <= 126 ? c : '?') - 32)].width * size;
		if (x > start && x + wordWidth > start + maxWidth) { x = start; y += line; }
		for (unsigned char character : word) {
			const unsigned char c = character >= 32 && character <= 126 ? character : '?';
			const Glyph& glyph = font.glyphs[static_cast<size_t>(c - 32)];
			const float w = glyph.width * size, h = font.height * size;
			const glm::vec4 uv = glyph.uv;
			vertices.insert(vertices.end(), {{{x, y}, {uv.x, uv.y}, color}, {{x + w, y}, {uv.z, uv.y}, color},
				{{x + w, y + h}, {uv.z, uv.w}, color}, {{x, y}, {uv.x, uv.y}, color},
				{{x + w, y + h}, {uv.z, uv.w}, color}, {{x, y + h}, {uv.x, uv.w}, color}});
			x += w;
		}
		x += font.glyphs[0].width * size;
	}
}
void Hud::Draw(const std::vector<Vertex>& vertices, const Font& font)
{
	glBindTexture(GL_TEXTURE_2D, font.texture);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
}

void Hud::Render(int width, int height, const HudInfo& info)
{
	bodyVertices.clear(); titleVertices.clear();
	const float scale = Scale(width, height), w = width / scale, h = height / scale;
 const float panelWidth=390, x=16, y=16;
 Rect(x,y,panelWidth,info.help ? 468.0f : 218.0f,Ink);
 Text("THE MIDNIGHT MISSION",x+14,y+10,0.61f,Gold);
 Text(info.clock + " / " + (info.story ? "STORY" : "MANUAL"),x+14,y+32,0.55f,Muted);
 Text(info.selection,x+14,y+54,0.58f,Cream,panelWidth-28,true);
 Text(info.description,x+14,y+82,0.56f,Muted,panelWidth-28);
 Text(info.story ? "N manual / Enter activate / Shift+N replay" : info.controls,
  x+14,y+119,0.55f,Cream,panelWidth-28);
 for (int i=0;i<9;++i) {
  const float bx=x+14+i*40;
  Rect(bx,y+160,35,22,(info.selected==i || (i==8 && info.selected>=8)) ? glm::vec4(0.38f,0.29f,0.16f,0.97f) : glm::vec4(0.12f,0.14f,0.16f,0.8f));
  Text(i<8 ? std::to_string(i+1) : "B",bx+12,y+163,0.52f,(info.selected==i || (i==8 && info.selected>=8)) ? Gold : Cream);
 }
 Text(info.mouseLook ? "Mouse-look / Esc cursor / H guide / G hide" : "M mouse-look / H guide / G hide / " + info.camera,x+14,y+191,0.53f,Muted);
 if (info.help) {
  const char* lines[]={
   "1 Woody / 2 Jessie / 3 Bullseye / 4 Buzz / 5 Car",
   "6 Ball / 7 Lamp / 8 Ghost / Ctrl+B blocks",
   "Selection pauses the story. N resumes; Shift+N replays.",
   "W/S move / A/D turn / Shift run / Space stop",
   "Jessie + Bullseye: R mount or dismount nearby",
   "Buzz: Q/E altitude / L laser / Z/X aim / Alt+click target",
   "Enter: activate car when the rescue party arrives",
   "M mouse-look / Esc cursor / C mode / Home reset",
   "No toy: W/A/S/D camera / PgUp-PgDn height / F orbit",
   "B rebuild blocks / P pause story / G hide all text",
   "F1 wireframe / F2 model / F3 textures / F4 rays",
   "Tab edit / T operation / J-L, U-O, I-K axes",
   "F12 screenshot / H close this guide",
   "` settings / Scroll zoom / Shift+F2 shading on-off"
  };
  float lineY=y+227;
  for (const char* line:lines) { Text(line,x+14,lineY,0.50f,Muted,panelWidth-28); lineY+=17; }
 }
 const float settingsX=w-226;
 Rect(w-128,16,112,28,Ink);
 Text("` Settings",w-116,22,0.56f,info.settingsOpen ? Gold : Cream);
 if (info.settingsOpen) {
  Rect(settingsX,50,210,176,Ink);
  Text("RENDER SETTINGS",settingsX+12,60,0.55f,Gold);
  const char* names[]={"Lighting","Shading","Ray tracing","Textures"};
  const bool values[]={info.lighting,info.shading,info.rayTracing,info.textures};
  for (int i=0;i<4;++i) {
   const float rowY=84+i*29.0f;
   Rect(settingsX+8,rowY,194,25,glm::vec4(0.12f,0.14f,0.16f,0.90f));
   Text(names[i],settingsX+16,rowY+4,0.56f,Cream);
   Text(values[i] ? "ON" : "OFF",settingsX+159,rowY+4,0.55f,values[i] ? Gold : Muted);
  }
  Text("Click to toggle / G hides everything",settingsX+12,204,0.44f,Muted,186);
 }
 if (info.mouseLook) {
  Rect(w*0.5f-5,h*0.5f-0.75f,10,1.5f,Gold);
  Rect(w*0.5f-0.75f,h*0.5f-5,1.5f,10,Gold);
 }
	const GLboolean depth = glIsEnabled(GL_DEPTH_TEST), blend = glIsEnabled(GL_BLEND), cull = glIsEnabled(GL_CULL_FACE);
	GLboolean depthMask; glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
	glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDepthMask(GL_FALSE);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	shader.Activate(); shader.SetVec2("uViewport", {w, h}); shader.SetInt("uAtlas", 0);
	glActiveTexture(GL_TEXTURE0); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, buffer);
	Draw(bodyVertices, body); Draw(titleVertices, title);
	glBindVertexArray(0); glDepthMask(depthMask);
	if (depth) glEnable(GL_DEPTH_TEST); if (!blend) glDisable(GL_BLEND); if (cull) glEnable(GL_CULL_FACE);
}

int Hud::HitTest(glm::vec2 mouse,int width,int height,bool settingsOpen) const
{
 const float w=width/Scale(width,height);
 mouse/=Scale(width,height);
 if (mouse.x>=w-128 && mouse.x<=w-16 && mouse.y>=16 && mouse.y<=44) return 100;
 if (settingsOpen && mouse.x>=w-218 && mouse.x<=w-24 && mouse.y>=84 && mouse.y<200) {
  const int row=static_cast<int>((mouse.y-84)/29);
  if (mouse.y<=84+row*29+25) return 101+row;
 }
 if (mouse.y<176 || mouse.y>198 || mouse.x<30) return -1;
 const int index=static_cast<int>((mouse.x-30)/40);
 if (index>8 || mouse.x>30+index*40+35) return -1;
 return index;
}
bool Hud::Covers(glm::vec2 mouse,int width,int height,bool help,bool settingsOpen) const
{
 const float w=width/Scale(width,height);
 mouse/=Scale(width,height);
 if (mouse.x>=w-128 && mouse.x<=w-16 && mouse.y>=16 && mouse.y<=44) return true;
 if (settingsOpen && mouse.x>=w-226 && mouse.x<=w-16 && mouse.y>=50 && mouse.y<=226) return true;
 return mouse.x>=16 && mouse.x<=406 && mouse.y>=16 && mouse.y<=16+(help ? 468 : 218);
}
