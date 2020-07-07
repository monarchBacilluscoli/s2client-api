#ifndef GRAPH_RENDERER_H
#define GRAPH_RENDERER_H

#include "../../../contrib/gnuplot-iostream.h"
#include <list>
#include <tuple>
#include <vector>

class GraphRenderer
{
protected:
    Gnuplot m_gp;

public:
    GraphRenderer() = default;
    virtual ~GraphRenderer() = default;
    void SetTitle(const std::string &title)
    {
        m_gp << "set title '" << title << "'" << std::endl;
    }
    // adds single quote on both sides of the words sent in
    static std::string SingleQuoteString(const std::string &words)
    {
        return "'" + words + "'";
    }
};

class GraphRenderer2D : public GraphRenderer
{
public:
    GraphRenderer2D() : GraphRenderer(){};
    virtual ~GraphRenderer2D() = default;
    void SetXLabel(const std::string &label)
    {
        m_gp << "set xlabel " << GraphRenderer::SingleQuoteString(label) << std::endl;
    }
    void SetYLabel(const std::string &label)
    {
        m_gp << "set ylabel " << GraphRenderer::SingleQuoteString(label) << std::endl;
    }
    void SetXRange(float min_range, float max_range)
    {
        m_gp << "set xrange [" << min_range << ":" << max_range << "]" << std::endl;
    }
};

class LineChartRenderer2D : public GraphRenderer2D
{
public:
    LineChartRenderer2D() : GraphRenderer2D()
    {
        m_gp << "set style func linespoints" << std::endl;
    };
    virtual ~LineChartRenderer2D() = default;

    // one TContainer one line
    template <typename T, template <typename, typename> class TContainer> // support at least both list and vector
    void Show(const TContainer<std::vector<T>, std::allocator<std::vector<T>>> &data_y, const std::vector<T> &data_x, const std::vector<std::string> &line_names)
    {
        int data_sz = data_y.size();
        m_gp << "plot";
        int i = 0;
        for (auto it = data_y.begin(); it != data_y.end() && i < data_sz; ++it, ++i)
        {
            m_gp << m_gp.file1d(std::make_tuple(data_x, *it)) << "with lines title " << GraphRenderer::SingleQuoteString(line_names[i]) << ",";
        }
        m_gp << std::endl;
    }
};

class ScatterRenderer2D : public GraphRenderer2D
{
public:
    ScatterRenderer2D() : GraphRenderer2D(){};
    virtual ~ScatterRenderer2D() = default;

    static int MaxGroupSize()
    {
        return g_max_group_count;
    }

    // One TContainer one group of points
    template <template <typename, typename> class TContainer>
    void Show(const TContainer<std::vector<std::vector<float>>, std::allocator<std::vector<std::vector<float>>>> &objs, const std::vector<std::string> &names)
    {
        size_t obj_group_sz = objs.size() < names.size() ? objs.size() : names.size();
        if (obj_group_sz > g_max_group_count)
        {
            throw("ScatterRenderer2D can not render so many groups of points, please check the ScatterRenderer2D::MaxGroupSize to get the max size@" + std::string(__FUNCTION__));
        }
        if (obj_group_sz != names.size())
        {
            throw("The size of names and objs group size must be equal@" + std::string(__FUNCTION__));
        }
        m_gp << "plot";
        int i = 0;
        for (auto it = objs.begin(); it != objs.end(); ++it)
        {
            m_gp << m_gp.file1d(*it) << "with points pt " << i + 1 << " title " << SingleQuoteString(names[i]) << ",";
            ++i;
        }
        m_gp << std::endl;
    }

    // template <template <typename, typename> class TContainer>
    // void Show(const TContainer<std::vector<std::vector<float>>, std::allocator<std::vector<std::vector<float>>>> &objs, const std::vector<std::string> &name)
    // {
    //     size_t obj_group_sz = objs.size();
    //     if (obj_group_sz > g_max_group_count)
    //     {
    //         throw("ScatterRenderer2D can not render so many groups of points, please check the ScatterRenderer2D::MaxGroupSize to get the max size@" + std::string(__FUNCTION__));
    //     }
    //     m_gp << "plot";
    //     int i = 0;
    //     for (auto it = objs.begin(); it != objs.end(); ++it)
    //     {
    //         m_gp << m_gp.file1d(*it) << "with points pt " << i + 1 << " title " << SingleQuoteString(names[i]) << ",";
    //         ++i;
    //     }
    //     m_gp << std::endl;
    // }

    void Show(const std::list<std::vector<std::vector<float>>>::iterator &begin, const std::list<std::vector<std::vector<float>>>::iterator &end, const std::vector<std::string> &names)
    {
        int group_size = std::distance(begin, end);
        if (group_size > g_max_group_count)
        {
            throw("ScatterRenderer2D can not render so many groups of points, please check the ScatterRenderer2D::MaxGroupSize to get the max size@" + std::string(__FUNCTION__));
        }
        if (group_size != names.size())
        {
            throw("The size of names and objs group size must be equal@" + std::string(__FUNCTION__));
        }
        m_gp << "plot";
        int i = 0;
        for (auto it = begin; it != end; ++it)
        {
            m_gp << m_gp.file1d(*it) << "with points pt " << i + 1 << " title " << SingleQuoteString(names[i]) << ",";
            ++i;
        }
        m_gp << std::endl;
    }

private:
    static const int g_max_group_count = 23; // depends on gnuplot
};

#endif // GRAPH_RENDERER_H