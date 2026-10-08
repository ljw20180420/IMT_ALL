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
    NODE *child;
    double boundary;

    SUBNODE(int *htp_, NODE *child_, double boundary_)
        : htp(htp_), child(child_), boundary(boundary_)
    {
    }

    ~SUBNODE()
    {
        if (child)
            delete child;
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

    std::deque<SUBNODE> get_best_subnodes(std::deque<int *> &anchors, int h, int t, int &total_node, double &total_split_length)
    {
        std::deque<SUBNODE> subnodes;
        if (h == t)
        {
            total_node = 0;
            total_split_length = 0;
            return subnodes;
        }
        else
        {
            total_node = maxint;
            total_split_length = -inf;
            for (int m = t - 1; m != h - 1; --m)
            {
                int total_node_left;
                double total_split_length_left;
                std::deque<SUBNODE> subnodes_left = get_best_subnodes(anchors, h, m, total_node_left, total_split_length_left);
                subnodes_left.emplace_back(anchors[m], new NODE(anchors[m], anchors[t], depth + 1), (std::abs(get_val(*anchors[m])) + std::abs(get_val(*(anchors[m] - 1)))) / 2);
                if (subnodes_left.back().child->legal)
                {
                    if (total_node_left != maxint)
                        total_node_left += subnodes_left.back().child->total_node;
                    total_split_length_left += subnodes_left.back().child->total_split_length;
                }
                else
                    break;
                if (total_node_left < total_node || (total_node_left == total_node && total_split_length_left > total_split_length))
                {
                    total_node = total_node_left;
                    total_split_length = total_split_length_left;
                    subnodes.swap(subnodes_left);
                }
            }
        }
        return subnodes;
    }

    NODE()
    {
    }

    NODE(int *hidx, int *tidx, int depth_)
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
            {
                end = &idxs[i];
                break;
            }
        if (!start)
        {
            total_split_length = 0;
            total_node = 1;
        }
        else
        {
            std::deque<int *> anchors = {start};
            for (int *pidx = start + 1; pidx != end; ++pidx)
                if (get_val(*(pidx - 1)) != get_val(*pidx))
                    anchors.push_back(pidx);
            anchors.push_back(end);

            subnodes = get_best_subnodes(anchors, 0, anchors.size() - 1, total_node, total_split_length);
            if (total_node == maxint)
            {
                legal = false;
                return;
            }
            else
            {
                double val_ll = std::abs(get_val(*(anchors.front() - 1)));
                double val_l = std::abs(get_val(*anchors.front()));
                double val_u = std::abs(get_val(*(anchors.back() - 1)));
                double val_uu = std::abs(get_val(*anchors.back()));
                subnodes.emplace_back(anchors.back(), (NODE *)NULL, (val_u + val_uu) / 2);
                ++total_node;
                total_split_length += (val_l - val_ll) + (val_uu - val_u);
            }
        }
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
        for (int i = 0; i < 50; ++i)
        {
            NODE::inputss.emplace_back();
            raat.generate_inputs(1);
        }
    }
    
    
    {
        std::vector<int> idxs;
        for (int i = 0; i < NODE::inputss.size(); ++i)
            idxs.push_back(i);
        NODE root(idxs.data(), idxs.data() + idxs.size(), 0);
        std::ofstream fout("root");
        std::deque<int> path = {0};
        root.list_boundaries(path, fout);
    }

    return 0;
}