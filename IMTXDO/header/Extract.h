#include <deque>
#include <climits>
#include <cmath>
#include <tuple>

enum Direction
{
    n,
    up,
    down,
    left,
    right,
    upleft,
    upright,
    downleft,
    downright
};
enum Button
{
    n = 0,
    a = 8,
    b = 4,
    c = 2,
    d = 1,
    ab = 12,
    ac = 10,
    ad = 9,
    bc = 6,
    bd = 5,
    cd = 3,
    abc = 14,
    abd = 13,
    acd = 11,
    bcd = 7,
    abcd = 15
};

struct Skill
{
    std::deque<Direction> directions;
    Button button;
    bool charge;
};

struct Combo
{
    Skill skill;
    int happen;
    std::deque<int> hits;
    std::deque<int> damages;
};

struct Show
{
    Direction direction;
    Button button;
    int duration;
    int hit;
    int damage;
};

struct Analyzer
{
    std::deque<Show> shows;
    std::deque<Combo> combos;

    void extract(int thres, int fail_UB)
    {
        std::deque<std::deque<double>> valss;
        for (int i = 0; i < shows.size();)
        {
            std::deque<double> vals;
            int val, valp;
            for (int j = 0, k, ii, happen_best, ed; j < combos.size() && i < shows.size(); ++j)
            {
                for (k = 0; k < combos[j].hits.size(); ++k, ++i)
                {
                    for (; i < shows.size(); ++i)
                    {
                        if (j == 0 && k == 0)
                        {
                            if (combos[0].hits[0] == shows[i].hit && combos[0].damages[0] == shows[i].damage)
                                break;
                        }
                        else
                        {
                            if (shows[i].hit > 0)
                                break;
                        }
                    }
                    if (k == 0)
                    {
                        valp = val;
                        val = 0;
                        ii = i;
                    }
                    if (i == shows.size() || combos[j].hits[k] != shows[i].hit || combos[j].damages[k] != shows[i].damage)
                        break;
                }
                if (k == combos[j].hits.size())
                {
                    std::tie(happen_best, ed) = get_happen_best(ii, combos[j], thres);
                    if (happen_best == INT_MAX)
                    {
                        vals.push_back(-1);
                        break;
                    }
                    else
                    {
                        if (j > 0)
                            vals.push_back(valp - happen_best);
                        val += happen_best;
                    }
                }
                else
                {
                    if (j > 0)
                    {
                        if (k > 0)
                        {
                            std::tie(happen_best, ed) = get_happen_best(ii, combos[j], thres);
                            if (happen_best == INT_MAX)
                                vals.push_back(-1);
                            else
                                vals.push_back(valp - happen_best);
                        }
                        else
                        {
                            int fail_dis = 0, jj;
                            for (jj = ed + 1; jj < shows.size(); ++jj)
                            {
                                fail_dis += shows[jj].duration;
                                if (shows[jj].button == Button::n || fail_dis > fail_UB)
                                    break;
                            }
                            for (; fail_dis <= fail_UB; ++jj)
                            {
                                if (jj + 1 == shows.size() || shows[jj + 1].button != Button::n)
                                    break;
                                fail_dis += shows[jj + 1].duration;
                            }
                            int st = jj + 1;
                            if (combos[j].skill.charge)
                                for (; fail_dis <= fail_UB; ++jj)
                                {
                                    if (jj + 1 == shows.size() || shows[jj + 1].button != combos[j].skill.button)
                                        break;
                                    fail_dis += shows[jj + 1].duration;
                                }
                            if (st == shows.size() || fail_dis > fail_UB || !right_input(st, combos[j], thres))
                                vals.push_back(-1);
                            else
                            {
                                val = 0;
                                for (int kk = std::min(ii,jj) + 1; kk <= std::max(ii,jj); ++kk)
                                    val += shows[kk].duration;
                                if (ii < jj)
                                    val = happen_best - 1 + val;
                                else
                                    val = happen_best - 1 - val;
                                if (!combos[j].skill.charge)
                                    ++val;
                                if (val <= fail_UB)
                                    vals.push_back(val);
                                else
                                    vals.push_back(-1);
                            }
                        }
                    }
                    break;
                }
            }
            if (vals.size() > 1 || !vals.empty() && vals.back() >= 0)
                valss.push_back(vals);
        }
    }

    std::pair<int, int> get_happen_best(int &ii, Combo &combo, int thres)
    {
        int happen_best = INT_MAX, current_idx = 0, eff, ed;
        while (ii >= 0)
        {
            std::tie(eff, ed) = find_next(ii, current_idx, combo, thres);
            if (eff >= 0 && std::abs(happen_best - combo.happen) > std::abs(eff - combo.happen))
                happen_best = eff;
        }
        return std::make_pair(happen_best, ed);
    }

    std::pair<int, int> find_next(int &ii, int &current_idx, Combo &combo, int thres)
    {
        int button_UB = 3 * combo.happen;
        if (combo.skill.charge)
            while (shows[ii].button != Button::n)
                current_idx += shows[ii--].duration;
        int st = -1, ed = -1, ipt, eff;
        for (; st < 0; --ii)
        {
            if (ed < 0)
            {
                if (current_idx > button_UB)
                    break;
                if (shows[ii].button == combo.skill.button)
                {
                    ed = ii;
                    eff = current_idx;
                }
            }
            current_idx += shows[ii].duration;
            if (ed >= 0)
            {
                if (ii == 0 || shows[ii - 1].button != combo.skill.button)
                {
                    st = ii;
                    ipt = current_idx;
                }
            }
        }
        if (combo.skill.charge)
            ++eff;
        else
            eff = ipt;
        if (ed < 0 || eff > button_UB)
        {
            ii = -1;
            return std::make_pair(-1, -1);
        }

        if (right_input(st, combo, thres))
            return std::make_pair(eff, ed);
        else
            return std::make_pair(-1, -1);
    }

    bool right_input(int st, Combo &combo, int thres)
    {
        if (combo.skill.directions.size() > 1)
        {
            bool flag = shows[st].direction == combo.skill.directions.back() ? true : false;
            int kk = combo.skill.directions.size() - 1;
            for (int jj = st - 1, ipt_prev = 0, ipt_now = 0; kk >= 0, jj >= 0; --jj)
            {
                if (flag && shows[jj].direction != combo.skill.directions[kk])
                {
                    --kk;
                    ipt_prev = ipt_now;
                }
                ipt_now += shows[jj].duration;
                if (ipt_now - ipt_prev > thres)
                    break;
                if (kk >= 0)
                    flag = shows[jj].direction == combo.skill.directions[kk] ? true : false;
                if (flag && jj == 0)
                    --kk;
            }
            if (kk >= 0)
                return false;
        }
        else
        {
            if (combo.skill.directions.front() != Direction::n && shows[st].direction != combo.skill.directions.front())
                return false;
        }
        return true;
    }
};