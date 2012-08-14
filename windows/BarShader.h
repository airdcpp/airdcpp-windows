#include "atlcoll.h"

class CBarShader
{
public:
	CBarShader(uint32_t dwHeight, uint32_t dwWidth, COLORREF crColor = 0, uint64_t dwFileSize = 1ui64);
	~CBarShader(void);

	//set the width of the bar
	void SetWidth(uint32_t width);

	//set the height of the bar
	void SetHeight(uint32_t height);

	//returns the width of the bar
	int GetWidth() const { return m_iWidth; }

	//returns the height of the bar
	int GetHeight() const { return m_iHeight; }

	//sets new file size and resets the shader
	void SetFileSize(uint64_t qwFileSize);

	//fills in a range with a certain color, new ranges overwrite old
	void FillRange(uint64_t qwStart, uint64_t qwEnd, COLORREF crColor);

	//fills in entire range with a certain color
	void Fill(COLORREF crColor);

	//draws the bar
	void Draw(CDC& dc, int iLeft, int iTop, int);

protected:
	void BuildModifiers();
	void FillRect(CDC& dc, LPCRECT rectSpan, COLORREF crColor);

	uint64_t	m_qwFileSize;
	uint32_t	m_iWidth;
	uint32_t	m_iHeight;
	double		m_dblPixelsPerByte;
	double		m_dblBytesPerPixel;

private:
	CRBMap<uint64_t, COLORREF> m_Spans;
	double	*m_pdblModifiers;
	byte	m_used3dlevel;
	bool	m_bIsPreview;
};

typedef struct tagHLSTRIPLE {
	DOUBLE hlstHue;
	DOUBLE hlstLightness;
	DOUBLE hlstSaturation;
} HLSTRIPLE;

const int MAX_SHADE = 44;
const int SHADE_LEVEL = 90;
const int blend_vector[MAX_SHADE] = {0, 8, 16, 20, 10, 4, 0, -2, -4, -6, -10, -12, -14, -16, -14, -12, -10, -8, -6, -4, -2, 0, 
1, 2, 3, 8, 10, 12, 14, 16, 14, 12, 10, 6, 4, 2, 0, -4, -10, -20, -16, -8, 0};

class OperaColors {
public:
	static inline BYTE getRValue(const COLORREF& cr) { return (BYTE)(cr & 0xFF); }
	static inline BYTE getGValue(const COLORREF& cr) { return (BYTE)(((cr & 0xFF00) >> 8) & 0xFF); }
	static inline BYTE getBValue(const COLORREF& cr) { return (BYTE)(((cr & 0xFF0000) >> 16) & 0xFF); }
	static double RGB2HUE(double red, double green, double blue);
	static RGBTRIPLE HUE2RGB(double m0, double m2, double h);
	static HLSTRIPLE RGB2HLS(BYTE red, BYTE green, BYTE blue);
	static inline HLSTRIPLE RGB2HLS(COLORREF c) { return RGB2HLS(getRValue(c), getGValue(c), getBValue(c)); }
	static RGBTRIPLE HLS2RGB(double hue, double lightness, double saturation);
	static inline RGBTRIPLE HLS2RGB(HLSTRIPLE hls) { return HLS2RGB(hls.hlstHue, hls.hlstLightness, hls.hlstSaturation); }
	static inline COLORREF RGB2REF(const RGBTRIPLE& c) { return RGB(c.rgbtRed, c.rgbtGreen, c.rgbtBlue); }
	static inline COLORREF blendColors(const COLORREF& cr1, const COLORREF& cr2, double balance = 0.5) {
		BYTE r1 = getRValue(cr1);
		BYTE g1 = getGValue(cr1);
		BYTE b1 = getBValue(cr1);
		BYTE r2 = getRValue(cr2);
		BYTE g2 = getGValue(cr2);
		BYTE b2 = getBValue(cr2);
		return RGB(
			(r1*(balance*2) + (r2*((1-balance)*2))) / 2,
			(g1*(balance*2) + (g2*((1-balance)*2))) / 2,
			(b1*(balance*2) + (b2*((1-balance)*2))) / 2
			);
	}
	static inline COLORREF brightenColor(const COLORREF& c, double brightness = 0) {
		if (brightness == 0) {
			return c;
		} else if (brightness > 0) {
			BYTE r = getRValue(c);
			BYTE g = getGValue(c);
			BYTE b = getBValue(c);
			return RGB(
				(r+((255-r)*brightness)),
				(g+((255-g)*brightness)),
				(b+((255-b)*brightness))
				);
		} else {
			return RGB(
				(getRValue(c)*(1+brightness)),
				(getGValue(c)*(1+brightness)),
				(getBValue(c)*(1+brightness))
				);
		}
	}
	static void FloodFill(CDC& hDC, int x1, int y1, int x2, int y2, COLORREF c1, COLORREF c2, bool light = true) {
		if (x2 <= x1 || y2 <= y1 || x2 > 10000)
			return;

		int w = x2 - x1;
		int h = y2 - y1;

		FloodCacheItem::FCIMapper fcim = {c1 & (light ? 0x80FF : 0x00FF), c2 & 0x00FF}; // Make it hash-safe
		FCIIter i = flood_cache.find(fcim);

		FloodCacheItem* fci = NULL;
		if (i != flood_cache.end()) {
			fci = i->second;
			if (fci->h >= h && fci->w >= w) {
				// Perfect, this kindof flood already exist in memory, lets paint it stretched
				SetStretchBltMode(hDC.m_hDC, HALFTONE);
				StretchBlt(hDC.m_hDC, x1, y1, w, h, fci->hDC, 0, 0, fci->w, fci->h, SRCCOPY);
				return;
			}
			DeleteDC(fci->hDC);
		} else {
			fci = new FloodCacheItem();
			flood_cache[fcim] = fci;
		}

		fci->hDC = ::CreateCompatibleDC(hDC.m_hDC);
		fci->w = w;
		fci->h = h;
		fci->mapper = fcim;
		
		BITMAPINFOHEADER bih;
		ZeroMemory(&bih, sizeof(BITMAPINFOHEADER));
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biWidth = w;
		bih.biHeight = -h;
		bih.biPlanes = 1;
		bih.biBitCount = 32;
		bih.biCompression = BI_RGB;
		bih.biClrUsed = 32;
		HBITMAP hBitmap = ::CreateDIBitmap(hDC.m_hDC, &bih, 0, NULL, NULL, DIB_RGB_COLORS);
		::DeleteObject(::SelectObject(fci->hDC, hBitmap));

		if (!light) {
			for (int _x = 0; _x < w; ++_x) {
				HBRUSH hBr = CreateSolidBrush(blendColors(c2, c1, (double)(_x - x1) / (double)(w)));
				RECT rc = { _x, 0, _x + 1, h };
				::FillRect(fci->hDC, &rc, hBr);
				DeleteObject(hBr);
			}
		} else {
			for (int _x = 0; _x <= w; ++_x) {
				COLORREF cr = blendColors(c2, c1, (double)(_x) / (double)(w));
				for (int _y = 0; _y < h; ++_y) {
					SetPixelV(fci->hDC, _x, _y, brightenColor(cr, (double)blend_vector[(size_t)floor(((double)(_y) / h) * (MAX_SHADE-1))] / (double)SHADE_LEVEL));
				}
			}
		}
		BitBlt(hDC.m_hDC, x1, y1, x2, y2, fci->hDC, 0, 0, SRCCOPY);
	}
	static void EnlightenFlood(const COLORREF& clr, COLORREF& a, COLORREF& b);
	static COLORREF TextFromBackground(COLORREF bg);

	static void ClearCache(); // A _lot_ easier than to clear certain cache items
	
private:
	struct FloodCacheItem {
		FloodCacheItem();
		~FloodCacheItem();

		struct FCIMapper {
			COLORREF c1;
			COLORREF c2;
		} mapper;

		int w;
		int h;
		HDC hDC;
	};

	struct fci_hash {
		size_t operator()(FloodCacheItem::FCIMapper __x) const { return (__x.c1 ^ __x.c2); }
		//bool operator()(const FloodCacheItem::FCIMapper& a, const FloodCacheItem::FCIMapper& b) {
		//	return a.c1 < b.c1 && a.c2 < b.c2;
		//};
	};
	
	struct fci_equal_to : public binary_function<FloodCacheItem::FCIMapper, FloodCacheItem::FCIMapper, bool> {
		bool operator()(const FloodCacheItem::FCIMapper& __x, const FloodCacheItem::FCIMapper& __y) const {
			return (__x.c1 == __y.c1) && (__x.c2 == __y.c2);
		}
	};

	typedef unordered_map<FloodCacheItem::FCIMapper, FloodCacheItem*, fci_hash, fci_equal_to> FCIMap;
	typedef FCIMap::iterator FCIIter;
	
	static FCIMap flood_cache;
};