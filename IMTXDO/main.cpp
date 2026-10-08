extern "C"
{
#include <xdo.h>
}
#include <list>
#include <deque>
#include <algorithm>
#include <set>
#include <map>
#include <iostream>

// repeat input
// merge two data
// from hits to command

struct INFORCE
{
    double val;
    int id;

    INFORCE(double val_, int id_)
    {
        val = val_;
        id = id_;
    }
};

struct comp_inforces
{
    bool operator()(const INFORCE &inforce1, const INFORCE &inforce2) const
    {
        return inforce1.val < inforce2.val;
    }
};

struct IMT;

struct SEG
{
    std::set<INFORCE, comp_inforces> inforces;
    IMT *imt_ptr = NULL;
};

struct NODE
{
    std::map<double, SEG> segs;
};

struct IMT
{
    int depth;
    std::deque<std::deque<double>> &valss;
    NODE root;

    IMT(int depth_, std::deque<std::deque<double>> &valss_, std::set<INFORCE, comp_inforces> &inforces_)
        : valss(valss_)
    {
        depth = depth_;
        std::set<INFORCE, comp_inforces> inforces;
        for (auto &inforce : inforces_)
            if (depth < valss[inforce.id].size() && valss[inforce.id][depth] >= 0)
                inforces.emplace(valss[inforce.id][depth], inforce.id);
        for (auto it = inforces.begin(); it != inforces.end(); ++it)
        {
            if (it == inforces.begin() || (depth < valss[it->id].size() - 1 && depth == valss[std::prev(it)->id].size() - 1) || (depth == valss[it->id].size() - 1 && depth < valss[std::prev(it)->id].size() - 1))
                root.segs.emplace_hint(root.segs.end(), it->val);
            root.segs.rbegin()->second.inforces.insert(root.segs.rbegin()->second.inforces.end(), *it);
        }
        for (auto &pair : root.segs)
            if (depth < valss_[pair.second.inforces.begin()->id].size() - 1)
                pair.second.imt_ptr = new IMT(depth + 1, valss, pair.second.inforces);
    }

    void insert(int id_)
    {
        std::map<double, SEG>::iterator itl;
        if (valss[id_][depth] < root.segs.begin()->first)
        {
            if ((depth < valss[id_].size() - 1 && depth < valss[root.segs.begin()->second.inforces.begin()->id].size() - 1) || (depth == valss[id_].size() - 1 && depth == valss[root.segs.begin()->second.inforces.begin()->id].size() - 1))
                itl = root.segs.begin();
            else
                itl = root.segs.emplace_hint(root.segs.begin(), valss[id_][depth]);
        }
        else
        {
            itl = root.segs.lower_bound(valss[id_][depth]);
            if (itl->second.inforces.rbegin()->val > valss[id_][depth] && (depth < valss[id_].size() - 1 && depth == valss[itl->second.inforces.begin()->id].size() - 1 || depth == valss[id_].size() - 1 && depth < valss[itl->second.inforces.begin()->id].size() - 1))
            {
                INFORCE inforce(valss[id_][depth], id_);
                auto itu = itl->second.inforces.upper_bound(inforce);
                itl = root.segs.emplace_hint(std::next(itl), itu->val, std::piecewise_construct, std::forward_as_tuple(itu, itl->second.inforces.end()));
                itl = root.segs.emplace_hint(itl, valss[id_][depth]);
                std::prev(itl)->second.inforces.erase(itu, std::prev(itl)->second.inforces.end());
                if (depth == valss[id_].size() - 1 && valss[std::prev(itl)->second.inforces.begin()->id][depth+1]>=0)
                {
                    delete std::prev(itl)->second.imt_ptr;
                    std::prev(itl)->second.imt_ptr = new IMT(depth + 1, valss, std::prev(itl)->second.inforces);
                    std::next(itl)->second.imt_ptr = new IMT(depth + 1, valss, std::next(itl)->second.inforces);
                }
            }
            else
            {
                if (itl->second.inforces.rbegin()->val < valss[id_][depth] && (depth < valss[id_].size() - 1 && depth == valss[itl->second.inforces.begin()->id].size() - 1 || depth == valss[id_].size() - 1 && depth < valss[itl->second.inforces.begin()->id].size() - 1))
                {
                    ++itl;
                    if (itl == root.segs.end())
                        itl = root.segs.emplace_hint(root.segs.end(), valss[id_][depth]);
                }
            }
        }
        itl->second.inforces.emplace(valss[id_][depth], id_);
        if (depth + 1 < valss[id_].size() && valss[id_][depth + 1] >= 0)
        {
            if (itl->second.imt_ptr)
                itl->second.imt_ptr->insert(id_);
            else
                itl->second.imt_ptr = new IMT(depth + 1, valss, itl->second.inforces);
        }
    }

    ~IMT()
    {
        for (auto &pair : root.segs)
            delete pair.second.imt_ptr;
    }
};

void test_xdo()
{
    const char *display = NULL;
    xdo_new(display);
    xdo_t *x = xdo_new(NULL);

    sleep(5);
    xdo_enter_text_window(x, CURRENTWINDOW, "Hallo xdo!", 500000);
    int i;
    for (i = 0; i < 8; i = i + 1)
    {
        xdo_send_keysequence_window(x, CURRENTWINDOW, "Left", 200000);
    }
    xdo_send_keysequence_window(x, CURRENTWINDOW, "BackSpace+e", 200000);
    xdo_free(x);
};

int main()
{

    return 0;
}