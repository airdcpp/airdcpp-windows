#include "StdAfx.h"
#include "barshader.h"

#include <cmath>

#include "../client/StringTokenizer.h"
#include "MainFrm.h"

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#define HALF(X) (((X) + 1) / 2)

CBarShader::CBarShader(uint32_t dwHeight, uint32_t dwWidth, COLORREF crColor /*= 0*/, uint64_t qwFileSize /*= 1*/)
{
	m_iWidth = dwWidth;
	m_iHeight = dwHeight;
	m_qwFileSize = 0;
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(qwFileSize, 0);
	m_pdblModifiers = NULL;
	m_bIsPreview = false;
	m_used3dlevel = 0;
	SetFileSize(qwFileSize);
}

CBarShader::~CBarShader(void)
{
	delete[] m_pdblModifiers;
}

void CBarShader::BuildModifiers()
{
	if (m_pdblModifiers != NULL)
	{
		delete[] m_pdblModifiers;
		m_pdblModifiers = NULL; // 'new' may throw an exception
	}

	static const double dDepths[5] = { 5.5, 4.0, 3.0, 2.50, 2.25 };		//aqua bar - smoother gradient jumps...
	double	depth = dDepths[((m_used3dlevel > 5) ? (256 - m_used3dlevel) : m_used3dlevel) - 1];
	uint32_t dwCount = HALF(static_cast<uint32_t>(m_iHeight));
	double piOverDepth = PI / depth;
	double base = PI / 2 - piOverDepth;
	double increment = piOverDepth / (dwCount - 1);

	m_pdblModifiers = new double[dwCount];
	for (uint32_t i = 0; i < dwCount; i++, base += increment)
		m_pdblModifiers[i] = sin(base);
}

void CBarShader::SetWidth(uint32_t width)
{
	if(m_iWidth != width) {
		m_iWidth = width;
		if (m_qwFileSize)
			m_dblPixelsPerByte = (double)m_iWidth / m_qwFileSize;
		else
			m_dblPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dblBytesPerPixel = (double)m_qwFileSize / m_iWidth;
		else
			m_dblBytesPerPixel = 0.0;
	}
}

void CBarShader::SetFileSize(uint64_t qwFileSize)
{
	if (m_qwFileSize != qwFileSize)
	{
		m_qwFileSize = qwFileSize;
		if (m_qwFileSize)
			m_dblPixelsPerByte = static_cast<double>(m_iWidth) / m_qwFileSize;
		else
			m_dblPixelsPerByte = 0.0;
		if (m_iWidth)
			m_dblBytesPerPixel = static_cast<double>(m_qwFileSize) / m_iWidth;
		else
			m_dblBytesPerPixel = 0.0;
	}
}

void CBarShader::SetHeight(uint32_t height)
{
	if(m_iHeight != height)
 	{
		m_iHeight = height;

		BuildModifiers();
	}
}

void CBarShader::FillRange(uint64_t qwStart, uint64_t qwEnd, COLORREF crColor)
{
	if(qwEnd > m_qwFileSize)
		qwEnd = m_qwFileSize;

	if(qwStart >= qwEnd)
		return;
	POSITION endprev, endpos = m_Spans.FindFirstKeyAfter(qwEnd + 1ui64);

	if ((endprev = endpos) != NULL)
		m_Spans.GetPrev(endpos);
	else
		endpos = m_Spans.GetTailPosition();

	COLORREF endcolor = m_Spans.GetValueAt(endpos);

	if ((endcolor == crColor) && (m_Spans.GetKeyAt(endpos) <= qwEnd) && (endprev != NULL))
		endpos = endprev;
	else
	endpos = m_Spans.SetAt(qwEnd, endcolor);

	for (POSITION pos = m_Spans.FindFirstKeyAfter(qwStart); pos != endpos;)
	{
		POSITION pos1 = pos;
		m_Spans.GetNext(pos);
		m_Spans.RemoveAt(pos1);
	}
	
	m_Spans.GetPrev(endpos);

	if ((endpos == NULL) || (m_Spans.GetValueAt(endpos) != crColor))
		m_Spans.SetAt(qwStart, crColor);
}

void CBarShader::Fill(COLORREF crColor)
{
	m_Spans.RemoveAll();
	m_Spans.SetAt(0, crColor);
	m_Spans.SetAt(m_qwFileSize, 0);
}

void CBarShader::Draw(CDC& dc, int iLeft, int iTop, int P3DDepth)
{
	m_used3dlevel = (byte)P3DDepth;
	COLORREF crLastColor = (COLORREF)~0, crPrevBkColor = dc.GetBkColor();
	POSITION pos = m_Spans.GetHeadPosition();
	RECT rectSpan;
	rectSpan.top = iTop;
	rectSpan.bottom = iTop + m_iHeight;
	rectSpan.right = iLeft;

	int64_t iBytesInOnePixel = static_cast<int64_t>(m_dblBytesPerPixel + 0.5);
	uint64_t qwStart = 0;
	COLORREF crColor = m_Spans.GetNextValue(pos);

	iLeft += m_iWidth;
	while ((pos != NULL) && (rectSpan.right < iLeft)) {
		uint64_t qwSpan = m_Spans.GetKeyAt(pos) - qwStart;
		int iPixels = static_cast<int>(qwSpan * m_dblPixelsPerByte + 0.5);

		if(iPixels > 0) {
			rectSpan.left = rectSpan.right;
			rectSpan.right += iPixels;
			FillRect(dc, &rectSpan, crLastColor = crColor);

			qwStart += static_cast<uint64_t>(iPixels * m_dblBytesPerPixel + 0.5);
		} else {
			double dRed = 0, dGreen = 0, dBlue = 0;
			uint32_t dwRed, dwGreen, dwBlue; 
			uint64_t qwLast = qwStart, qwEnd = qwStart + iBytesInOnePixel;

			do {
				double	dblWeight = (min(m_Spans.GetKeyAt(pos), qwEnd) - qwLast) * m_dblPixelsPerByte;
				dRed   += GetRValue(crColor) * dblWeight;
				dGreen += GetGValue(crColor) * dblWeight;
				dBlue  += GetBValue(crColor) * dblWeight;
				if ((qwLast = m_Spans.GetKeyAt(pos)) >= qwEnd)
					break;
				crColor = m_Spans.GetValueAt(pos);
				m_Spans.GetNext(pos);
			} while(pos != NULL);
			rectSpan.left = rectSpan.right;
			rectSpan.right++;

		//	Saturation
			dwRed = static_cast<uint32_t>(dRed);
			if (dwRed > 255)
				dwRed = 255;
			dwGreen = static_cast<uint32_t>(dGreen);
			if (dwGreen > 255)
				dwGreen = 255;
			dwBlue = static_cast<uint32_t>(dBlue);
			if (dwBlue > 255)
				dwBlue = 255;

			FillRect(dc, &rectSpan, crLastColor = RGB(dwRed, dwGreen, dwBlue));
			qwStart += iBytesInOnePixel;
		}
		while((pos != NULL) && (m_Spans.GetKeyAt(pos) <= qwStart))
			crColor = m_Spans.GetNextValue(pos);
		}
	if ((rectSpan.right < iLeft) && (crLastColor != ~0))
	{
		rectSpan.left = rectSpan.right;
		rectSpan.right = iLeft;
		FillRect(dc, &rectSpan, crLastColor);
	}
	dc.SetBkColor(crPrevBkColor);	//restore previous background color
}

void CBarShader::FillRect(CDC& dc, LPCRECT rectSpan, COLORREF crColor)
{
	if(!crColor)
		dc.FillSolidRect(rectSpan, crColor);
	else
{
		if (m_pdblModifiers == NULL)
			BuildModifiers();

		double	dblRed = GetRValue(crColor), dblGreen = GetGValue(crColor), dblBlue = GetBValue(crColor);
		double	dAdd, dMod;

		if (m_used3dlevel > 5)		//Cax2 aqua bar
		{
			dMod = 1.0 - .025 * (256 - m_used3dlevel);		//variable central darkness - from 97.5% to 87.5% of the original colour...
			dAdd = 255;

			dblRed = dMod * dblRed - dAdd;
			dblGreen = dMod * dblGreen - dAdd;
			dblBlue = dMod * dblBlue - dAdd;
		}
		else
			dAdd = 0;

		RECT		rect;
		int			iTop = rectSpan->top, iBot = rectSpan->bottom;
		double		*pdCurr = m_pdblModifiers, *pdLimit = pdCurr + HALF(m_iHeight);

		rect.right = rectSpan->right;
		rect.left = rectSpan->left;

		for (; pdCurr < pdLimit; pdCurr++)
		{
			crColor = RGB( static_cast<int>(dAdd + dblRed * *pdCurr),
				static_cast<int>(dAdd + dblGreen * *pdCurr),
				static_cast<int>(dAdd + dblBlue * *pdCurr) );
			rect.top = iTop++;
			rect.bottom = iTop;
			dc.FillSolidRect(&rect, crColor);

			rect.bottom = iBot--;
			rect.top = iBot;
		//	Fast way to fill, background color is already set inside previous FillSolidRect
			dc.FillSolidRect(&rect, crColor);
		}
	}
}

// OperaColors
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define MIN3(a, b, c) (((a) < (b)) ? ((((a) < (c)) ? (a) : (c))) : ((((b) < (c)) ? (b) : (c))))
#define MAX3(a, b, c) (((a) > (b)) ? ((((a) > (c)) ? (a) : (c))) : ((((b) > (c)) ? (b) : (c))))
#define CENTER(a, b, c) ((((a) < (b)) && ((a) < (c))) ? (((b) < (c)) ? (b) : (c)) : ((((b) < (a)) && ((b) < (c))) ? (((a) < (c)) ? (a) : (c)) : (((a) < (b)) ? (a) : (b))))
#define ABS(a) (((a) < 0) ? (-(a)): (a))

OperaColors::FCIMap OperaColors::flood_cache;

double OperaColors::RGB2HUE(double r, double g, double b) {
	double H;
	double m2 = MAX3(r, g, b);
	double m1 = CENTER(r, g, b);
	double m0 = MIN3(r, g, b);

	if (m2 == m1) {
		if (r == g) {
			H = 60;
			goto _RGB2HUE_END;
		}
		if (g == b) {
			H = 180;
			goto _RGB2HUE_END;
		}
		H = 60;
		goto _RGB2HUE_END;
	}
	double F = 60 * (m1 - m0) / (m2 - m0);
	if (r == m2) {
		H = 0 + F * (g - b);
		goto _RGB2HUE_END;
	}
	if (g == m2) {
		H = 120 + F * (b - r);
		goto _RGB2HUE_END;
	}
	H = 240 + F * (r - g);

_RGB2HUE_END:
	if (H < 0)
		H = H + 360;
	return H;
}

RGBTRIPLE OperaColors::HUE2RGB(double m0, double m2, double h) {
	RGBTRIPLE rgbt = {0, 0, 0};
	double m1, F;
	int n;
	while (h < 0)
		h += 360;
	while (h >= 360)
		h -= 360;
	n = (int)(h / 60);

	if (h < 60)
		F = h;
	else if (h < 180)
		F = h - 120;
	else if (h < 300)
		F = h - 240;
	else
		F = h - 360;
	m1 = m0 + (m2 - m0) * sqrt(ABS(F / 60));
	switch (n) {
		case 0: rgbt.rgbtRed = (BYTE)(255*m2); rgbt.rgbtGreen = (BYTE)(255*m1); rgbt.rgbtBlue = (BYTE)(255*m0); break;
		case 1: rgbt.rgbtRed = (BYTE)(255*m1); rgbt.rgbtGreen = (BYTE)(255*m2); rgbt.rgbtBlue = (BYTE)(255*m0); break;
		case 2: rgbt.rgbtRed = (BYTE)(255*m0); rgbt.rgbtGreen = (BYTE)(255*m2); rgbt.rgbtBlue = (BYTE)(255*m1); break;
		case 3: rgbt.rgbtRed = (BYTE)(255*m0); rgbt.rgbtGreen = (BYTE)(255*m1); rgbt.rgbtBlue = (BYTE)(255*m2); break;
		case 4: rgbt.rgbtRed = (BYTE)(255*m1); rgbt.rgbtGreen = (BYTE)(255*m0); rgbt.rgbtBlue = (BYTE)(255*m2); break;
		case 5: rgbt.rgbtRed = (BYTE)(255*m2); rgbt.rgbtGreen = (BYTE)(255*m0); rgbt.rgbtBlue = (BYTE)(255*m1); break;
	}
	return rgbt;
}

HLSTRIPLE OperaColors::RGB2HLS(BYTE red, BYTE green, BYTE blue) {
	double r = (double)red / 255;
	double g = (double)green / 255;
	double b = (double)blue / 255;
	double m0 = MIN3(r, g, b), m2 = MAX3(r, g, b), d;
	HLSTRIPLE hlst = {0, -1, -1};

	hlst.hlstLightness = (m2 + m0) / 2;
	d = (m2 - m0) / 2;
	if (hlst.hlstLightness <= 0.5) {
		if(hlst.hlstLightness == 0) hlst.hlstLightness = 0.1;
		hlst.hlstSaturation = d / hlst.hlstLightness;
	} else {
		if(hlst.hlstLightness == 1) hlst.hlstLightness = 0.99;
		hlst.hlstSaturation = d / (1 - hlst.hlstLightness);
	}
	if (hlst.hlstSaturation > 0 && hlst.hlstSaturation < 1)
		hlst.hlstHue = RGB2HUE(r, g, b);
	return hlst;
}

RGBTRIPLE OperaColors::HLS2RGB(double hue, double lightness, double saturation) {
	RGBTRIPLE rgbt = {0, 0, 0};
	double d;

	if (lightness == 1) {
		rgbt.rgbtRed = rgbt.rgbtGreen = rgbt.rgbtBlue = 255; return rgbt;
	}
	if (lightness == 0)
		return rgbt;
	if (lightness <= 0.5)
		d = saturation * lightness;
	else
		d = saturation * (1 - lightness);
	if (d == 0) {
		rgbt.rgbtRed = rgbt.rgbtGreen = rgbt.rgbtBlue = (BYTE)(lightness * 255); return rgbt;
	}
	return HUE2RGB(lightness - d, lightness + d, hue);
}

void OperaColors::EnlightenFlood(const COLORREF& clr, COLORREF& a, COLORREF& b) {
	HLSCOLOR hls_a = ::RGB2HLS(clr);
	HLSCOLOR hls_b = hls_a;
	BYTE buf = HLS_L(hls_a);
	if (buf < 38)
		buf = 0;
	else
		buf -= 38;
	a = ::HLS2RGB(HLS(HLS_H(hls_a), buf, HLS_S(hls_a)));
	buf = HLS_L(hls_b);
	if (buf > 217)
		buf = 255;
	else
		buf += 38;
	b = ::HLS2RGB(HLS(HLS_H(hls_b), buf, HLS_S(hls_b)));
}

COLORREF OperaColors::TextFromBackground(COLORREF bg) {
	HLSTRIPLE hlst = RGB2HLS(bg);
	if (hlst.hlstLightness > 0.63)
		return RGB(0, 0, 0);
	else
		return RGB(255, 255, 255);
}

void OperaColors::ClearCache() {
	FCIIter i = flood_cache.begin();
	for (; !flood_cache.empty(); i = flood_cache.begin()) {
		FloodCacheItem* fci = i->second;
		flood_cache.erase(i);
		delete fci;
	}
}

OperaColors::FloodCacheItem::FloodCacheItem() : w(0), h(0), hDC(NULL) { }

OperaColors::FloodCacheItem::~FloodCacheItem() {
	if (hDC) {
		DeleteDC(hDC);
	}
}
