#include <deque>
#include <climits>
#include <cmath>
#include <tuple>
#include <vector>
#include <algorithm>

struct SKILL
{
    static int chain_inter;
    std::deque<int> directions;
    int button;
    bool charge;
    int hit;
    int damage;
};

int SKILL::chain_inter;

struct INFORCE
{
    static std::deque<SKILL> skills;
    static std::deque<int> combo_acum_hits, combo_acum_damages;
    double start;
    double end;
    std::deque<int> types;

    static void asign_skills(std::deque<SKILL> &skills_)
    {
        skills = skills_;
        combo_acum_hits.resize(1, skills[0].hit);
        combo_acum_damages.resize(1, skills[0].damage);
        for (int i = 1; i < skills.size(); ++i)
        {
            combo_acum_hits.push_back(combo_acum_hits[i-1] + skills[i].hit);
            combo_acum_damages.push_back(combo_acum_damages[i-1] + skills[i].damage);
        }
    }

    double get_happen(int type)
    {
        return skills[type].charge ? end : start; 
    }
};

std::deque<SKILL> INFORCE::skills;
std::deque<int> INFORCE::combo_acum_hits, INFORCE::combo_acum_damages;

struct MARKER
{
    double start;
    double end;
    int type;
};



std::deque<std::deque<double>> extract(std::deque<INFORCE> &inforces, std::deque<std::deque<MARKER>> &markerss, double radius_lower, double radius_upper)
{
    std::deque<std::deque<double>> inputss;
    std::vector<int> midxs(markerss.size(), 0);
    for (int s = 0; s < inforces.size(); ++s)
    {
        if (!std::binary_search(inforces[s].types.begin(), inforces[s].types.end(), 0))
            continue;
        for (int i = s, type = 0; i <= inforces.size(); ++i)
        {
            bool correct_type = type == 0 || (i < inforces.size() && std::binary_search(inforces[i].types.begin(), inforces[i].types.end(), type));
            bool allinmarker = true;
            if (correct_type)
            {
                for (int j = 0; j < markerss.size(); ++j)
                {
                    for (; midxs[j] < markerss[j].size() && markerss[j][midxs[j]].end + radius_upper < inforces[i].end; ++midxs[j])
                        ;
                    bool inmarker = false;
                    for (int k = midxs[j]; k < markerss[j].size() && markerss[j][k].start - radius_lower <= inforces[i].start; ++k)
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
            if (correct_type && allinmarker)
                ++type;
            else if (type > 1 || (type == 1 && correct_type))
            {
                inputss.emplace_back();
                if (correct_type)
                    inputss.back().push_front(inforces[i - 1].get_happen(type - 1) - inforces[i].get_happen(type));
                --type;
                for (int j = i - 1; type > 0; --type, --j)
                    inputss.back().push_front(inforces[j].get_happen(type) - inforces[j - 1].get_happen(type - 1));
                break;
            }
        }
    }
    return inputss;
}

void record()
{
    /* record directions */;
    /* record buttons */;
    /* record is_reds */;
    /* record frames */;
    /* record acum_hits */;
    /* record acum_damages */;
}

void recognize()
{
    /* record directions */;
    /* record buttons */;
    /* record is_reds */;
    /* record frames */;
    /* record acum_hits */;
    /* record acum_damages */;
}

std::deque<MARKER> types2markers(std::deque<int> &types)
{
    std::deque<MARKER> markers;
    for (int i = 1; i < types.size() - 1; ++i)
    {
        if (types[i-1] != types[i])
        {
            markers.emplace_back();
            markers.back().type = types[i];
            markers.back().start = i;
        }
        if (types[i] != types[i+1])
            markers.back().end = i;
    }
    markers.pop_back();
    return markers;
}

template <typename T>
int search_type(T begin, T end, int val)
{
    auto lb = lower_bound (begin, end, val);
    if (lb == end || *lb > val)
        return -1;
    else
        std::distance(begin, lb);
}

bool is_sub_button(int button1, int button2)
{
    while(button1 > 0)
    {
        if (button1 % 2 > button2 % 2)
            return false;
        button1 = button1 / 2;
        button2 = button2 / 2;
    }
    return true;
}

bool can_be_type(SKILL &skill, std::deque<int> &ndp_directions, int button, int i)
{
    if (skill.button != button)
        return false;
    if (skill.directions.size() == 1 && skill.directions.front() != ndp_directions[i])
        return false;
    int l = skill.directions.size() - 1;
    for (int j = i, k = i; l >= 0 && k > 0; --k)
    {
        if (j - k > SKILL::chain_inter)
            return false;
        if (ndp_directions[k] == skill.directions[l] && (l > 0 && ndp_directions[k-1] == skill.directions[l-1] || l == 0 && ndp_directions[k-1] != ndp_directions[k]))
        {
            j = k;
            --l;
        }
    }
    if (l >= 0)
        return false;
    return true;
}

void notify(std::deque<int> &directions, std::deque<int> &buttons, std::deque<bool> &is_reds, std::deque<int> &frames, std::deque<int> &acum_hits, std::deque<int> &acum_damages)
{
    std::deque<int> ndp_directions;
    std::deque<int> ndp_buttons;
    std::deque<bool> ndp_is_reds;
    std::deque<int> hit_types, damage_types;
    for (int i = 0; i < frames.size(); ++i)
        if (i == 0 || frames[i-1] != frames[i])
        {
            ndp_directions.push_back(directions[i]);
            ndp_buttons.push_back(buttons[i]);
            ndp_is_reds.push_back(is_reds[i]);
            hit_types.push_back(search_type(INFORCE::combo_acum_hits.begin(), INFORCE::combo_acum_hits.end(), acum_hits[i]));
            damage_types.push_back(search_type(INFORCE::combo_acum_damages.begin(), INFORCE::combo_acum_damages.end(), acum_damages[i]));
        }
    std::deque<MARKER> hit_markers = types2markers(hit_types);
    std::deque<MARKER> damage_markers = types2markers(damage_types);

    std::deque<INFORCE> inforces;
    std::deque<int> alternative_types = {0};
    int fi, li;
    for (fi = 0; fi < ndp_buttons.size() && ndp_buttons[fi] != 0 && !ndp_is_reds[fi]; ++fi);
    for (li = ndp_buttons.size() - 1; li >= 0 && ndp_buttons[li] != 0 && !ndp_is_reds[li]; --li);
    for (int i = fi, end, next; i <= li; ++i)
    {
        if (ndp_is_reds[i] || ndp_buttons[i] == 0)
            continue;

        for (end = i+1; end <= li && is_sub_button(ndp_buttons[i], ndp_buttons[end]); ++end);
        for (next = end; next <= li && ndp_buttons[next] != 0 && !ndp_is_reds[next]; ++next);

        inforces.emplace_back();
        inforces.back().start = i;
        inforces.back().end = end;
        for (int type : alternative_types)
            if (can_be_type(INFORCE::skills[type], ndp_directions, ndp_buttons[i], i))
                inforces.back().types.push_back(type);
        
        alternative_types.resize(1, 0);
        for (int type : inforces.back().types)
            if (type + 1 < INFORCE::skills.size())
                alternative_types.push_back(type + 1);

        i = next - 1;
    }
}