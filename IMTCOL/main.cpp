// #include <Afxwin.h>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <numeric>
#include <thread>
#include <iostream>
#include <fstream>
#include <deque>

class CMainWindow : public CFrameWnd
{
    struct Record
    {
        std::deque<CRect> rects;
        std::deque<CBitmap> bitmaps;
        std::deque<CDC> dcMems;
        CClientDC dcScr;
        std::deque<Matrix> *matricess;
        int output_idx = 0;
        int nContinue = 0;

        Record()
        : dcScr(NULL)
        {
            dcScr.SelectObject(BLACK_PEN);
            dcScr.SelectObject(NULL_BRUSH);
        }

        void set_rectangles()
        {
            bitmaps.clear();
            dcMems.clear();
            for (int i = 0; i < rects.size(); ++i)
            {
                dcMems.emplace_back();
                dcMems.back().CreateCompatibleDC(&dcScr);
                bitmaps.emplace_back();
                bitmaps.back().CreateDiscardableBitmap(&dcScr, rects[i].Width(), rects[i].Height());
                dcMems.back().SelectObject(&bitmaps.back());
            }
        }

        void do(LPVOID pParam)
        {
            matricess = new std::deque<Matrix>[sz];
            std::deque<CWinThread *> pThreads;
            std::deque<std::pair<std::deque<Matrix> *, int>> sPairs;
            while (nContinue)
            {
                for (int i = 0; i < rects.size(); ++i)
                    dcMems.BitBlt(0, 0, rects[i].Width(), rects[i].Height(), dcScr, rects[i].left, rects[i].top, SRCCOPY);
                for (int i = 0; i < rects.size(); ++i)
                    matricess[i].emplace_back(bitmaps[i]);
                if (matricess[0].size() > 1000)
                {
                    sPairs.emplace_back(matricess, output_idx++);
                    pThreads.push_back(AfxBeginThread(&sPairs.back()));
                    matricess = new std::deque<Matrix>[sz];
                }
                ::Sleep(10);
            }
            sPairs.emplace_back(matricess, output_idx++);
            pThreads.push_back(AfxBeginThread(save, &sPairs.back()));
            for (CWinThread *pThread : pThreads)
                ::WaitForSingleObject(pThread->m_hThread, INFINITE);
        }

        void save(LPVOID pParam)
        {
            std::pair<std::deque<Matrix> *, int> *ppair = (std::pair<std::deque<Matrix> *, std::ofstream *> *)pParam;
            std::ofstream fout(std::to_string(ppair->second));
            fout.write((char *)&sz, sizeof(sz));
            int len = ppair->first[0].size();
            fout.write((char *)&len, sizeof(len));
            for (int i = 0; i < sz; ++i)
                for (Matrix &matrix : ppair->first[i])
                    matrix.save(fout);
            delete[] ppair->first;
        }
    } record;

    static int cxChar = LOWORD(GetDialogBaseUnits());
    static int cyChar = HIWORD(GetDialogBaseUnits());
    CButton m_switchPushButton;
    CButton m_drawCheckBox;
    BOOL bStartDraw = FALSE;
    CPoint point_start, point_end;
    CWinThread *pThread;

public:
    CMainWindow()
    {
        Create(NULL, _T ("Recorder"));
        m_switchPushButton.Create(_T ("Open"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(cxChar, cyChar, 8 * cxChar, 3 * cyChar), this, 0);
        m_drawCheckBox.Create(_T ("Draw"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, CRect(cxChar, 4 * cyChar, 8 * cxChar, 6 * cyChar), this, 1);
    }

    ~CMainWindow()
    {
        if (record.nContinue)
            OnswitchPushButtonClicked();
        dcScr.SetROP2(R2_NOT);
        for (CRect &rect : record.rects)
            dcScr.Rectangle(rect);
    }

protected:
    afx_msg void OnswitchPushButtonClicked()
    {
        if (record.rects.empty() || m_drawCheckBox.GetCheck == BST_CHECKED)
            return;
        if (!record.nContinue)
        {
            record.set_rectangles();
            record.nContinue = 1;
            pThread = AfxBeginThread(record.do, NULL);
            m_switchPushButton.SetWindowText(_T("Close"));
        }
        else
        {
            record.nContinue = 0;
            ::WaitForSingleObject(pThread->m_hThread, INFINITE);
            m_switchPushButton.SetWindowText(_T("Open"));
        }
    }

    afx_msg void OndrawCHECKBOXClicked()
    {
        if (m_drawCheckBox.GetCheck == BST_CHECKED)
        {
            ::ReleaseCapture();
            m_drawCheckBox.SetCheck(BST_UNCHECKED);
        }
        else
        {
            dcScr.SetROP2(R2_NOT);
            for (CRect &rect : record.rects)
                dcScr.Rectangle(rect);
            record.rects.clear();
            dcScr.SetROP2(R2_COPYPEN);
            SetCapture();
            m_drawCheckBox.SetCheck(BST_CHECKED);
        }
    }

    afx_msg void OnLbuttondown(UINT nFlags, CPoint point)
    {
        if (record.nContinue)
            OnswitchPushButtonClicked();
        CRect rect;
        m_drawCheckBox.GetClientRect(rect);
        if (GetCapture() == this && m_drawCheckBox.GetCheck == BST_CHECKED && (rect.left > point.x || rect.right < point.x || rect.top > point.y || rect.botton < point.y))
        {
            bStartDraw = TRUE;
            point_start = point;
        }
    }

    afx_msg void OnLbuttonup(UINT nFlags, CPoint point)
    {
        if (bStartDraw)
        {
            point_end = point;
            record.rects.emplace_back(std::min(point_start.x, point_end.x), std::min(point_start.y, point_end.y), std::max(point_start.x, point_end.x), std::max(point_start.y, point_end.y));
            dcScr.Rectangle(rect);
            bStartDraw = FALSE;
        }
    }
    DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CMainWindow, CFrameWnd)
ON_BN_CLICKED(0, OnswitchPushButtonClicked)
ON_BN_CLICKED(1, OndrawCHECKBOXClicked)
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

class CMyApp : public CWinApp
{
public:
    virtual BOOL InitInstance()
    {
        m_pMainWnd = new CMainWindow;
        m_pMainWnd->ShowWindow(m_nCmdShow);
        m_pMainWnd->UpdateWindow(); // this send WM_PAINT to m_pMainWnd
        return TRUE;
    }
} myApp;

struct Distri
{
    int xA2direction;
    int yA2direction;
    int xButton2direction;
    int xButtonInter;
    int yButton2direction;
    int xframe2direction;
    int xframeInter;
    int yframe2direction;
    int xhit2direction;
    int xhitInter;
    int yhit2direction;
    int xdamage2direction;
    int xdamageInter;
    int ydamage2direction;

    void rescale(double xfactor, double yfactor)
    {
        xA2direction = xA2direction * xfactor;
        yA2direction = yA2direction * yfactor;
        xButton2direction = xButton2direction * xfactor;
        xButtonInter = xButtonInter * xfactor;
        yButton2direction = yButton2direction * yfactor;
        xframe2direction = xframe2direction * xfactor;
        xframeInter = xframeInter * xfactor;
        yframe2direction = yframe2direction * yfactor;
        xhit2direction = xhit2direction * xfactor;
        xhitInter = xhitInter * xfactor;
        yhit2direction = yhit2direction * yfactor;
        xdamage2direction = xdamage2direction * xfactor;
        xdamageInter = xdamageInter * xfactor;
        ydamage2direction = ydamage2direction * yfactor;
    }
};

struct Matrix
{
    std::vector<COLORREF> colors;
    int cx;

    Matrix(int cx_, int cy_)
        : cx(cx_)
    {
        colors.resize(cx_ * cy_);
    }

    Matrix(CBitmap &bitmap)
    {
        BITMAP bBITMAP;
        bitmap.GetBitmap(bBITMAP);
        for (int y = 0; y < bBITMAP.bmHeight; ++y)
            for (int x = 0; x < bBITMAP.bmWidth; ++x)
            {
                int64_t l = y * bBITMAP.bmWidthBytes * 3 + x * 3;
                colors.push_back(RGB(bBITMAP.bmBits[l], bBITMAP.bmBits[l + 1], bBITMAP.bmBits[l + 2]));
            }
        cx = bBITMAP.bmWidth;
    }

    COLORREF &operator()(int x, int y)
    {
        return colors[y * cx + x];
    }

    bool same_color(COLORREF color1, COLORREF color2, uint8_t thres)
    {
        bool same_red = GetRValue(color1) >= GetRValue(color2) && GetRValue(color1) - GetRValue(color2) < thres || GetRValue(color1) < GetRValue(color2) && GetRValue(color2) - GetRValue(color1) < thres;
        bool same_green = GetGValue(color1) >= GetGValue(color2) && GetGValue(color1) - GetGValue(color2) < thres || GetGValue(color1) < GetGValue(color2) && GetGValue(color2) - GetGValue(color1) < thres;
        bool same_blue = GetBValue(color1) >= GetBValue(color2) && GetBValue(color1) - GetBValue(color2) < thres || GetBValue(color1) < GetBValue(color2) && GetBValue(color2) - GetBValue(color1) < thres;
        return same_red && same_green && same_blue;
    }

    void stretch(int lcm_cx, int lcm_cy)
    {
        int cy = colors.size() / cx;
        if (lcm_cx == cx && lcm_cy == cy)
            return;
        int sx = lcm_cx / cx, sy = lcm_cy / cy;
        Matrix matrix_stretch(lcm_cx, lcm_cy);
        for (int y = 0; y < cy; ++y)
            for (int x = 0; x < cx; ++x)
                for (int j = 0; j < sy; ++j)
                    for (int i = 0; i < sx; ++i)
                    {
                        int ly = y * sy + j, lx = x * sx + i;
                        matrix_stretch(lx, ly) = this->operator()(x, y);
                    }
        swap(matrix_stretch);
    }

    std::tuple<CPoint, double> search(Matrix &matrix, uint8_t thres, CRect rect)
    {
        int mcy = matrix.colors.size() / matrix.cx;
        int shx = rect.Width() - matrix.cx + 1, shy = rect.Height() - mcy + 1;
        std::vector<double> same_percents(shx * shy);

        double max_same_percent = -1;
        CPoint mpoint = rect.TopLeft();
        for (int lx = rect.left; lx < rect.left + shx; ++lx)
            for (int ly = rect.top; ly < rect.top + shy; ++ly)
            {
                uint64_t same_num = 0, total_num = 0;
                for (int x = 0; x < matrix.cx; ++x)
                    for (int y = 0; y < mcy; ++y)
                    {
                        COLORREF color_target = this->operator()(lx + x, ly + y);
                        COLORREF color_search = matrix(x, y);
                        if (GetRValue(color_search) + GetGValue(color_search) + GetBValue(color_search) > 0)
                        {
                            ++total_num;
                            if (same_color(color_target, color_search, thres))
                                ++same_num;
                        }
                    }
                double new_same_percent = double(same_num) / total_num;
                if (new_same_percent > max_same_percent)
                {
                    max_same_percent = new_same_percent;
                    mpoint.x = lx;
                    mpoint.y = ly;
                }
            }

        return std::make_tuple(mpoint, max_same_percent);
    }

    void swap(Matrix &matrix)
    {
        colors.swap(matrix.colors);
        std::swap(cx, matrix.cx);
    }

    void save(std::ofstream &fout)
    {
        fout.write((char *)&cx, sizeof(cx));
        int64_t sz = colors.size();
        fout.write((char *)&sz, sizeof(sz));
        for (COLORREF color : colors)
            fout.write((char *)&color, sizeof(color));
    }

    void load(std::ifstream &fin)
    {
        fin.read((char *)&cx, sizeof(cx));
        int64_t sz;
        fin.read((char *)&sz, sizeof(sz));
        colors.resize(sz);
        for (COLORREF color : colors)
            fin.read((char *)&color, sizeof(color));
    }
};

struct Sample
{
    Matrix matrix;
    std::deque<CPoint> points;

    Sample(CBitmap &bitmap)
        : matrix(bitmap)
    {
    }

    std::deque<double> match(Matrix &target, uint8_t thres)
    {
        std::deque<double> percents;
        for (CPoint point : points)
        {
            double percent;
            std::tie(point, percent) = target.search(matrix, thres, CRect(point.x, point.y, point.x + 1, point.y + 1));
            percents.push_back(percent);
        }
        return percents;
    }
};

std::deque<Sample> load_bitmaps(std::string name, int num)
{
    std::deque<Sample> samples;
    for (int i = 0; i < num; ++i)
    {
        CBitmap bitmap;
        bitmap.LoadBitmap(("bitmaps/" + name + std::itos(i)).c_str());
        samples.emplace_back(bitmap);
    }
    return samples;
}

bool get_sample_points(std::deque<Sample> &frame_samples, std::deque<Sample> &hit_samples, std::deque<Sample> &damage_samples, std::deque<Sample> &direction_samples, Sample &A_sample, Sample &B_sample, Sample &C_sample, Sample &D_sample, std::deque<Matrix> &input_matrices, uint8_t thres, double percent_thres, CRect input_rect, CRect hit_rect, CRect damage_rect, Distri distri)
{
    bool locate_direction = false, locate_A = false;
    CPoint point;
    std::deque<CPoint> direction_points;
    double percent;
    for (Matrix &matrix : input_matrices)
    {
        if (direction_points.empty())
            for (Sample &sample : direction_samples)
            {
                std::tie(point, percent) = matrix.search(sample.matrix, thres, CRect(0, 0, matrix.cx, matrix.data.size() / matrix.cx));
                if (percent > percent_thres)
                {
                    direction_points.push_back(point);
                    break;
                }
            }
        if (!locate_A)
        {
            std::tie(point, percent) = matrix.search(A_sample.matrix, thres, CRect(0, 0, matrix.cx, matrix.data.size() / matrix.cx));
            if (percent > percent_thres)
            {
                A_sample.points = std::deque<CPoint>{point};
                locate_A = true;
            }
        }
        if (!direction_points.empty() && locate_A)
            break;
    }
    if (direction_points.empty() || !locate_A)
        return false;
    for (Sample &sample : direction_samples)
        sample.points = direction_points;
    double xfactor = (A_sample.points[0].x - direction_points[0].x) / double(distri.xA2direction);
    double yfactor = (A_sample.points[0].y - direction_points[0].y) / double(distri.yA2direction);
    distri.rescale(xfactor, yfactor);
    B_sample.points = std::deque<CPoint>{CPoint(direction_points[0].x + distri.xButton2direction, direction_points[0].y + distri.yButton2direction)};
    C_sample.points = std::deque<CPoint>{CPoint(B_sample.points[0].x + distri.xButtonInter, B_sample.points[0].y)};
    D_sample.points = std::deque<CPoint>{CPoint(C_sample.points[0].x + distri.xButtonInter, C_sample.points[0].y)};
    std::deque<CPoint> frame_points;
    frame_points.emplace_back(direction_points[0].x + distri.xframe2direction, direction_points[0].y + distri.yframe2direction);
    for (int i = 1; i < 3; ++i)
        frame_points.emplace_back(frame_points.back().x + distri.xframeInter, frame_points.back().y);
    for (Sample &sample : frame_samples)
        sample.points = frame_points;
    std::deque<CPoint> hit_points;
    hit_points.emplace_back(direction_points[0].x + distri.xhit2direction + input_rect.left - hit_rect.left, direction_points[0].y + distri.yhit2direction + input_rect.top - hit_rect.top);
    for (int i = 1; i < 2; ++i)
        hit_points.emplace_back(hit_points.back().x + distri.xhitInter, hit_points.back().y);
    for (Sample &sample : hit_samples)
        sample.points = hit_points;
    std::deque<CPoint> damage_points;
    damage_points.emplace_back(direction_points[0].x + distri.xhit2direction + input_rect.left - damage_rect.left, direction_points[0].y + distri.yhit2direction + input_rect.top - damage_rect.top);
    for (int i = 1; i < 2; ++i)
        damage_points.emplace_back(damage_points.back().x + distri.xhitInter, damage_points.back().y);
    for (Sample &sample : damage_samples)
        sample.points = damage_points;
    return true;
}

int get_number(std::deque<Sample> &samples, Matrix &matrix, uint8_t thres, double percent_thres)
{
    std::deque<double> max_percents = samples[0].match(matrix, thres);
    std::deque<int> max_idxs(max_percents.size(), 0);
    for (int i = 1; i < samples.size(); ++i)
    {
        std::deque<double> percents = samples[i].match(matrix, thres);
        for (int j = 0; j < percents.size(); ++j)
            if (percents[j] > max_percents[j])
            {
                max_percents[j] = percents[j];
                max_idxs[j] = i;
            }
    }
    int k;
    for (k = max_percents.size() - 1; k >= 0 && max_percents[k] > percent_thres; --k)
        ;
    if (k == max_percents.size() - 1)
        return 0;
    else
    {
        int number = max_idxs[k + 1];
        for (int i = k + 2; i < max_percents.size(); ++i)
            number = 10 * number + max_idxs[i];
        return number;
    }
}

bool decode(std::deque<Sample> &frame_samples, std::deque<Sample> &hit_samples, std::deque<Sample> &damage_samples, std::deque<Sample> &direction_samples, Sample &A_sample, Sample &B_sample, Sample &C_sample, Sample &D_sample, std::deque<Matrix> &input_matrices, std::deque<Matrix> &hit_matrices, std::deque<Matrix> &damage_matrices, uint8_t thres, double percent_thres)
{
    std::deque<int> frames, directions, buttons, hits, damages;
    for (Matrix &matrix : input_matrices)
    {
        frames.push_back(get_number(frame_samples, matrix, thres, percent_thres));
        directions.push_back(get_number(direction_samples, matrix, thres, percent_thres));
        int button = 0;
        if (A_sample.match(matrix, thres).front() > percent_thres)
            button += 8;
        if (B_sample.match(matrix, thres).front() > percent_thres)
            button += 4;
        if (C_sample.match(matrix, thres).front() > percent_thres)
            button += 2;
        if (D_sample.match(matrix, thres).front() > percent_thres)
            button += 1;
        buttons.push_back(button);
    }
    for (Matrix &matrix : hit_matrices)
        hits.push_back(get_number(hit_samples, matrix, thres, percent_thres));
    for (Matrix &matrix : damage_matrices)
        damages.push_back(get_number(damage_samples, matrix, thres, percent_thres));
}