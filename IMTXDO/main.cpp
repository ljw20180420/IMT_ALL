extern "C"
{
#include <xdo.h>
}
#include <list>
#include <deque>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <random>

// repeat input
// merge two data
// from hits to command

constexpr static const double inf = std::numeric_limits<double>::infinity();
std::default_random_engine generator;

struct NODE
{
    static std::deque<std::deque<double>> inputss;

    std::vector<std::pair<double, int>> inforces;
    std::deque<int> starts;
    std::deque<NODE *> children;
    int depth;

    std::deque<double> boundaries;

    void update_boundaries()
    {
        boundaries.resize(starts.size() - 1);
        for (int i = 1; i < starts.size(); ++i)
            boundaries[i - 1] = (inforces[starts[i] - 1].first + inforces[starts[i]].first) / 2;
        for (auto child : children)
            child->update_boundaries();
    }

    void random_boundaries(double UBUB, int numUB, int left_depth)
    {
        if (left_depth > 0)
        {
            std::uniform_real_distribution<double> uni_real_dis1(0, UBUB);
            std::uniform_real_distribution<double> uni_real_dis2(0, UBUB - uni_real_dis1(generator));
            int num = rand() % (numUB - 1) + 2;
            boundaries.clear();
            while (boundaries.size() < num)
            {
                for (int i = 0; i < num; ++i)
                    boundaries.push_back(uni_real_dis2.max() - uni_real_dis2(generator));
                std::sort(boundaries.begin(), boundaries.end());
                boundaries.resize(std::distance(boundaries.begin(), std::unique(boundaries.begin(), boundaries.end())));
            }
            children.clear();
            for (int i = 0; i < boundaries.size() - 1; ++i)
            {
                children.push_back(new NODE);
                children.back()->random_boundaries(UBUB, numUB, left_depth - 1);
            }
        }
        else
            return;
    }

    void generate_inputs(double up_dangle)
    {
        if (boundaries.empty())
        {
            inputss.back().push_back(-1);
            return;
        }
        int in = rand() % (boundaries.size() + 1);
        double LB = in > 0 ? boundaries[in - 1] : 0;
        double UB = in < boundaries.size() ? boundaries[in] : boundaries.back() + up_dangle;
        std::uniform_real_distribution<double> uni_real_dis(LB, UB);
        inputss.back().push_back(uni_real_dis(generator));
        if (0 < in && in < boundaries.size())
            children[in - 1]->generate_inputs(up_dangle);
    }

    void list_boundaries(std::deque<int> &path, std::ofstream &fout)
    {
        for (auto pa : path)
            fout << pa << ',';
        fout << '\n';
        for (auto boundary : boundaries)
            fout << boundary << ',';
        fout << '\n';
        for (int i = 0; i < children.size(); ++i)
        {
            path.push_back(i);
            children[i]->list_boundaries(path, fout);
            path.pop_back();
        }
    }

    bool isfail(int i)
    {
        return inforces[i].second == -1 || inputss[inforces[i].second].size() - 1 == depth;
    }

    void reset()
    {
        starts.clear();
        for (auto child : children)
            delete child;
        children.clear();
        for (int i = 1; i < inforces.size(); ++i)
            if (isfail(i - 1) && !isfail(i) || !isfail(i - 1) && isfail(i))
                starts.push_back(i);
        if (starts.size() > 2 || starts.size() == 2 && (inforces[starts[0] - 1].first == inforces[starts[0]].first || inforces[starts[1] - 1].first == inforces[starts[1]].first))
        {
            std::cerr << "mixed\n";
            exit(0);
        }
        else if (starts.size() == 2)
        {
            std::deque<int> anchors;
            for (int i = starts[0] + 1; i < starts[1]; ++i)
                if (inforces[i - 1].first != inforces[i].first)
                    anchors.push_back(i);
            anchors.push_front(starts[0]);
            anchors.push_back(starts[1]);
            std::deque<std::deque<double>> discrim(anchors.size() - 1);
            for (int i = 0; i < anchors.size() - 1; ++i)
            {
                std::array<double, 4> tmp{-inf, 0, 0, inf};
                bool mixed = false;
                for (int j = i + 1; j < anchors.size(); ++j)
                {
                    for (int k = anchors[j - 1]; k < anchors[j]; ++k)
                    {
                        if (depth + 1 < inputss[inforces[k].second].size())
                        {
                            double val = inputss[inforces[k].second][depth + 1];
                            if (depth + 1 == inputss[inforces[k].second].size() - 1)
                            {
                                if (tmp[1] == 0)
                                    continue;
                                if (val < tmp[1])
                                    tmp[0] = val > tmp[0] ? val : tmp[0];
                                else if (val > tmp[2])
                                    tmp[3] = val < tmp[3] ? val : tmp[3];
                                else
                                    mixed = true;
                            }
                            else
                            {
                                if (tmp[1] == 0)
                                {
                                    tmp[1] = tmp[2] = val;
                                    for (int l = anchors[i]; l < k; ++l)
                                    {
                                        double val_l = inputss[inforces[l].second][depth + 1];
                                        if (val_l < tmp[1])
                                            tmp[0] = val_l > tmp[0] ? val_l : tmp[0];
                                        else if (val_l > tmp[2])
                                            tmp[3] = val_l < tmp[3] ? val_l : tmp[3];
                                        else
                                        {
                                            mixed = true;
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    if (val <= tmp[0] || val >= tmp[3])
                                        mixed = true;
                                    else
                                    {
                                        tmp[1] = val < tmp[1] ? val : tmp[1];
                                        tmp[2] = val > tmp[2] ? val : tmp[2];
                                    }
                                }
                            }
                        }
                        if (mixed)
                            break;
                    }
                    if (mixed)
                        break;
                    if (tmp[1] == 0)
                        discrim[i].push_back(0);
                    else
                    {
                        double disleft = tmp[1] - tmp[0];
                        disleft = disleft == inf ? 0 : disleft;
                        double disright = tmp[3] - tmp[2];
                        disright = disright == inf ? 0 : disright;
                        discrim[i].push_back(disleft + disright);
                    }
                }
            }
            int min_part = 0;
            for (int i = 0; i < discrim.size(); ++min_part)
            {
                if (discrim[i].size() == 0)
                {
                    std::cerr << "violent model\n";
                    exit(0);
                }
                i += discrim[i].size();
            }
            if (min_part > 1)
            {
                std::deque<std::vector<std::pair<double, int>>> right_fix(discrim.size() - 1, std::vector<std::pair<double, int>>(min_part - 1, std::make_pair(-inf, -1)));
                for (int j = 0; j < right_fix.size(); ++j)
                    for (int mp = 0; mp < min_part - 1; ++mp)
                    {
                        if (mp == 0)
                        {
                            if (j < discrim[0].size())
                            {
                                right_fix[j][mp].first = discrim[0][j];
                                right_fix[j][mp].second = 0;
                            }
                        }
                        else
                            right_fix[j][mp] = DP(right_fix, discrim, j, mp);
                    }
                std::pair<double, int> right_fix_end = DP(right_fix, discrim, discrim.size() - 1, min_part - 1);
                starts.pop_front();
                for (int mp = min_part - 1, max_left = right_fix_end.second;; max_left = right_fix[max_left - 1][mp].second)
                {
                    starts.push_front(anchors[max_left]);
                    --mp;
                    if (mp < 0)
                        break;
                }
            }
        }
        starts.push_front(0);
        for (int i = 1; i < starts.size() - 1; ++i)
            children.push_back(new NODE(depth + 1, inforces.data() + starts[i], starts[i + 1] - starts[i]));
    }

    std::pair<double, int> DP(std::deque<std::vector<std::pair<double, int>>> &right_fix, std::deque<std::deque<double>> &discrim, int j, int mp)
    {
        double max_dis = -inf;
        int max_left;
        for (int i = j - 1; i >= 0; --i)
            if (j - i - 1 < discrim[i + 1].size())
            {
                if (right_fix[i][mp - 1].first + discrim[i + 1][j - i - 1] > max_dis)
                {
                    max_dis = right_fix[i][mp - 1].first + discrim[i + 1][j - i - 1];
                    max_left = i + 1;
                }
            }
            else
                break;
        return std::make_pair(max_dis, max_left);
    }

    NODE()
    {
    }

    NODE(int depth_, std::pair<double, int> *seg, int seg_sz)
    {
        depth = depth_;
        inforces.emplace_back(0, -1);
        for (int i = 0; i < seg_sz; ++i)
            if (depth < inputss[seg[i].second].size() && inputss[seg[i].second][depth] > 0)
                inforces.emplace_back(inputss[seg[i].second][depth], seg[i].second);
        inforces.emplace_back(inf, -1);
        std::sort(inforces.begin(), inforces.end());
        reset();
    }

    bool insert(int id)
    {
        if (depth >= inputss[id].size() || inputss[id][depth] <= 0)
            return true;
        auto inforce = std::make_pair(inputss[id][depth], id);
        auto ins_it = std::upper_bound(inforces.begin(), inforces.end(), inforce);
        int idx =  std::distance(inforces.begin(), ins_it);
        if (starts.size() == 1)
        {
            inforces.insert(ins_it, inforce);
            if (depth < inputss[id].size() - 1)
            {
                starts.push_back(idx);
                starts.push_back(idx + 1);
            }
            children.push_back(new NODE(depth + 1, inforces.data() + idx, 1));
            return true;
        }
        auto it = std::upper_bound(starts.begin(), starts.end(), idx);
        int idx1 = *(--it), cidx1 = std::distance(starts.begin(), it), idx0, cidx0;
        if (idx1 > 0)
        {
            idx0 = *(--it);
            cidx0 = std::distance(starts.begin(), it);
        }
        std::array<int, 2> try_cidx;
        if (idx1 == 0 || inforces[idx1].first <= inforce.first || idx0 == 0 && depth < inputss[id].size() - 1 || idx0 > 0 && depth == inputss[id].size() - 1)
            try_cidx[0] = -1;
        else
            try_cidx[0] = cidx0;
        if (idx1 > 0 && inforces[idx1 - 1].first >= inforce.first || idx1 > 0 && idx1 < starts.back() && depth == inputss[id].size() - 1 || (idx1 == 0 || idx1 == starts.back()) && depth < inputss[id].size() - 1)
            try_cidx[1] = -1;
        else
            try_cidx[1] = cidx1;
        if (try_cidx[0] < 0 && try_cidx[1] < 0)
            return false;
        if (idx1 == 0 || std::abs(inforces[idx1].first - inforce.first) <= std::abs(inforces[idx1 - 1].first - inforce.first))
            std::swap(try_cidx[0], try_cidx[1]);
        int cidx_apply = -1;
        if (try_cidx[0] == 0 || try_cidx[0] == children.size() + 1 || try_cidx[0] > 0 && try_cidx[0] < children.size() + 1 && children[try_cidx[0] - 1]->insert(id))
            cidx_apply = try_cidx[0];
        else if (try_cidx[1] == 0 || try_cidx[1] == children.size() + 1 || try_cidx[1] > 0 && try_cidx[1] < children.size() + 1 && children[try_cidx[1] - 1]->insert(id))
            cidx_apply = try_cidx[1];
        inforces.insert(ins_it, inforce);
        if (cidx_apply >= 0)
            for (int i = cidx_apply + 1; i < starts.size(); ++i)
                ++starts[i];
        else
            reset();
        return true;
    }

    ~NODE()
    {
        for (NODE *child : children)
            if (child)
                delete child;
    }
};

std::deque<std::deque<double>> NODE::inputss;

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
    NODE raat;
    raat.random_boundaries(10, 3, 3);
    std::ofstream fout("raat");
    std::deque<int> path;
    raat.list_boundaries(path, fout);
    path.clear();
    fout.close();

    std::vector<std::pair<double, int>> inforces;
    for (int i = 0; i < 1000; ++i)
    {
        NODE::inputss.emplace_back();
        raat.generate_inputs(1);
        inforces.emplace_back(0, i);
    }

    NODE root(0, inforces.data(), inforces.size());
    root.update_boundaries();
    fout.open("root");
    root.list_boundaries(path, fout);
    path.clear();
    fout.close();

    NODE root2(0, inforces.data(), 0);
    for (int id = 0; id < NODE::inputss.size(); ++id)
    {
        if(!root2.insert(id))
        {
            std::cerr << "insert fail\n";
            exit(0);
        }
    }
    root2.update_boundaries();
    fout.open("root2");
    root2.list_boundaries(path, fout);
    path.clear();
    fout.close();

    return 0;
}