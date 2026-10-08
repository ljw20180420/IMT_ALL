#include <list>
#include <deque>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <random>
#include <functional>
#include <memory>

// repeat input
// merge two data
// from hits to command

constexpr static const double inf = std::numeric_limits<double>::infinity();
constexpr static const int maxint = std::numeric_limits<int>::max();
std::default_random_engine generator;

struct NODE;

struct SUBNODE
{
    int *htp;
    std::unique_ptr<NODE> child;
    double boundary;

    SUBNODE(int *htp_, NODE *child_, double boundary_)
        : htp(htp_), child(child_), boundary(boundary_)
    {
    }
};

struct NODE
{
    static std::deque<std::deque<double>> inputss;

    std::vector<int> idxs;
    std::deque<SUBNODE> subnodes;
    int depth;
    int total_node;
    double total_split_length;
    bool legal;

    void random_boundaries(double UBLB, double UBUB, int numUB, int left_depth)
    {
        std::uniform_real_distribution<double> uni_real_dis1(UBLB, UBUB);
        std::uniform_real_distribution<double> uni_real_dis2(0, uni_real_dis1(generator));
        int num = (left_depth > 0) ? (rand() % (numUB - 1) + 2) : 2;
        std::set<double> boundaries;
        while (boundaries.size() < num)
            boundaries.insert(uni_real_dis2.max() - uni_real_dis2(generator));
        subnodes.clear();
        for (auto it = boundaries.begin(); it != boundaries.end(); ++it)
        {
            if (left_depth > 0 && it != prev(boundaries.end()))
            {
                subnodes.emplace_back((int *)NULL, new NODE, *it);
                subnodes.back().child->random_boundaries(UBLB, UBUB, numUB, left_depth - 1);
            }
            else
                subnodes.emplace_back((int *)NULL, (NODE *)NULL, *it);
        }
    }

    int binary_search(double val)
    {
        int l = 0, r = subnodes.size() - 1;
        while (r - l != 1)
        {
            int m = (l + r) / 2;
            if (subnodes[m].boundary > val)
                r = m;
            else
                l = m;
        }
        return l;
    }

    void generate_inputs(double up_dangle)
    {
        std::uniform_real_distribution<double> uni_real_dis(0, subnodes.back().boundary + up_dangle);
        double val = uni_real_dis.max() - uni_real_dis(generator);
        int l = -1;
        if (val < subnodes.front().boundary || val >= subnodes.back().boundary)
            val = -val;
        else if (subnodes.front().child)
            l = binary_search(val);
        inputss.back().push_back(val);
        if (l != -1)
            subnodes[l].child->generate_inputs(up_dangle);
    }

    void list_boundaries(std::deque<int> &path, std::ofstream &fout)
    {
        for (int pa : path)
            fout << pa << ',';
        fout << '\n';
        for (SUBNODE &subnode : subnodes)
            fout << subnode.boundary << ',';
        fout << '\n';
        for (int i = 0; i < subnodes.size(); ++i)
        {
            path.push_back(i);
            if (subnodes[i].child)
                subnodes[i].child->list_boundaries(path, fout);
            path.pop_back();
        }
    }

    double get_val(int idx)
    {
        if (idx == -1)
            return 0;
        else if (idx == -2)
            return -inf;
        else
            return inputss[idx][depth];
    }

    struct COMPARE_IDX
    {
        NODE *pnode;

        COMPARE_IDX(NODE *pnode_)
            : pnode(pnode_)
        {
        }

        bool operator()(int idx1, int idx2)
        {
            return std::abs(pnode->get_val(idx1)) < std::abs(pnode->get_val(idx2));
        }
    };

    NODE()
    {
    }

    NODE(int *hidx, int *tidx, int depth_, int depth_max)
        : depth(depth_)
    {
        idxs.push_back(-1);
        for (int *pidx = hidx; pidx != tidx; ++pidx)
            if (depth < inputss[*pidx].size())
                idxs.push_back(*pidx);
        idxs.push_back(-2);
        std::sort(idxs.begin(), idxs.end(), NODE::COMPARE_IDX(this));
        int *start = NULL, *end;
        legal = true;
        for (int i = 1; i < idxs.size(); ++i)
            if (get_val(idxs[i - 1]) <= 0 && get_val(idxs[i]) > 0)
            {
                if (!start)
                    start = &idxs[i];
                else
                {
                    legal = false;
                    return;
                }
            }
            else if (get_val(idxs[i - 1]) > 0 && get_val(idxs[i]) < 0)
                end = &idxs[i];
        if (!start)
        {
            total_split_length = 0;
            total_node = 1;
        }
        else if (depth == depth_max)
        {
            double val_ll = std::abs(get_val(*(start - 1)));
            double val_l = std::abs(get_val(*start));
            double val_u = std::abs(get_val(*(end - 1)));
            double val_uu = std::abs(get_val(*end));
            subnodes.emplace_back(start, (NODE *)NULL, (val_ll + val_l) / 2);
            subnodes.emplace_back(end, (NODE *)NULL, (val_u + val_uu) / 2);
            total_split_length = (val_l - val_ll) + (val_uu - val_u);
            total_node = 1;
        }
        else
        {
            std::deque<int *> anchors = {start};
            for (int *pidx = start + 1; pidx != end; ++pidx)
                if (get_val(*(pidx - 1)) != get_val(*pidx))
                    anchors.push_back(pidx);
            anchors.push_back(end);
            std::deque<double> boundaries;
            for (int *pidx : anchors)
                boundaries.push_back((std::abs(get_val(*(pidx - 1))) + std::abs(get_val(*pidx))) / 2);

            std::deque<std::deque<SUBNODE>> DP_subnodess;
            std::vector<std::pair<int, double>> best_total_node_split_length = {{0, 0.0}};
            std::vector<int> best_mid(anchors.size());
            for (int j = 1; j < anchors.size(); ++j)
            {
                DP_subnodess.emplace_back();
                best_total_node_split_length.emplace_back(maxint, -inf);
                for (int i = j - 1; i >= 0; --i)
                {
                    DP_subnodess.back().emplace_back(anchors[i], new NODE(anchors[i], anchors[j], depth + 1, depth_max), boundaries[i]);
                    if (!DP_subnodess.back().back().child->legal)
                        break;
                    if (best_total_node_split_length[i].first == maxint)
                        continue;
                    int total_node_new = best_total_node_split_length[i].first + DP_subnodess.back().back().child->total_node;
                    int total_split_length_new = best_total_node_split_length[i].second + DP_subnodess.back().back().child->total_split_length;
                    if (total_node_new < best_total_node_split_length[j].first || (total_node_new == best_total_node_split_length[j].first && total_split_length_new > best_total_node_split_length[j].second))
                    {
                        best_total_node_split_length[j].first = total_node_new;
                        best_total_node_split_length[j].second = total_split_length_new;
                        best_mid[j] = i;
                    }
                }
            }
            if (best_total_node_split_length.back().first == maxint)
            {
                legal = false;
                return;
            }
            else
            {
                total_node = best_total_node_split_length.back().first + 1;
                for (int j = anchors.size() - 1; j != 0; j = best_mid[j])
                    subnodes.push_front(std::move(DP_subnodess[j - 1][j - best_mid[j] - 1]));
                double val_ll = std::abs(get_val(*(start - 1)));
                double val_l = std::abs(get_val(*start));
                double val_u = std::abs(get_val(*(end - 1)));
                double val_uu = std::abs(get_val(*end));
                subnodes.emplace_back(anchors.back(), (NODE *)NULL, boundaries.back());
                total_split_length += (val_l - val_ll) + (val_uu - val_u);
            }
        }
    }

    void record()
    {
    }

    void unique()
    {
    }

    void notify()
    {
    }

    struct INFORCE
    {
        double time;
        int type;
    };

    std::deque<std::deque<double>> extract(std::deque<INFORCE> inforces, std::deque<std::deque<INFORCE>> markerss, int radius)
    {
        std::deque<std::deque<double>> inputss;
        std::vector<int> midxs(markerss.size(), 0);
        for (int i = 0, type = 0; i <= inforces.size(); ++i)
        {
            bool allinmarker = true;
            if (i < inforces.size() && inforces[i].type == type)
            {
                for (int j = 0; j < markerss.size(); ++j)
                {
                    for (; midxs[j] < markerss[j].size() && markerss[j][midxs[j]].time + radius < inforces[i].time; ++midxs[j])
                        ;
                    bool inmarker = false;
                    for (int k = midxs[j]; k < markerss[j].size() && markerss[j][k].time - radius <= inforces[i].time; ++k)
                        if (markerss[j][k].type == type)
                        {
                            inmarker = true;
                            break;
                        }
                    if (!inmarker)
                    {
                        allinmarker = false;
                        break;
                    }
                }
            }
            if (i < inforces.size() && allinmarker)
                ++type;
            else if (type >0 || (i < inforces.size() && inforces[i].type == type))
            {
                inputss.emplace_back();
                if (i < inforces.size() && inforces[i].type == type)
                    inputss.back().push_front(-inforces[i].time);
                for (int j = i - 1; j >= i - type; --j)
                    inputss.back().push_front(inforces[j].time);
                type = 0;
            }
        }
        return inputss;
    }
};

std::deque<std::deque<double>> NODE::inputss;

int main()
{
    {
        NODE raat;
        raat.random_boundaries(5, 10, 3, 3);
        std::ofstream fout("raat");
        std::deque<int> path = {0};
        raat.list_boundaries(path, fout);
        for (int i = 0; i < 400; ++i)
        {
            NODE::inputss.emplace_back();
            raat.generate_inputs(1);
        }
    }

    {
        std::vector<int> idxs;
        for (int i = 0; i < NODE::inputss.size(); ++i)
            idxs.push_back(i);
        NODE root(idxs.data(), idxs.data() + idxs.size(), 0, 3);
        std::ofstream fout("root");
        std::deque<int> path = {0};
        root.list_boundaries(path, fout);
    }

    return 0;
}