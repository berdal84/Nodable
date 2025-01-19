#include "BezierCurveSegment2D.h"

using namespace tools;

static void PathBezierCubicCurveToCasteljau(std::vector<Vec2>* path, const BezierCurveSegment2D& curv, float tess_tol, int level )
{
    float dx = curv.p4.x - curv.p1.x;
    float dy = curv.p4.y - curv.p1.y;
    float d2 = (curv.p2.x - curv.p4.x) * dy - (curv.p2.y - curv.p4.y) * dx;
    float d3 = (curv.p3.x - curv.p4.x) * dy - (curv.p3.y - curv.p4.y) * dx;
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        path->emplace_back(curv.p4.x, curv.p4.y);
    }
    else if (level < 10)
    {
        float x12 = (curv.p1.x + curv.p2.x) * 0.5f, y12 = (curv.p1.y + curv.p2.y) * 0.5f;
        float x23 = (curv.p2.x + curv.p3.x) * 0.5f, y23 = (curv.p2.y + curv.p3.y) * 0.5f;
        float x34 = (curv.p3.x + curv.p4.x) * 0.5f, y34 = (curv.p3.y + curv.p4.y) * 0.5f;
        float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
        float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
        float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;
        PathBezierCubicCurveToCasteljau(path, BezierCurveSegment2D{curv.p1.x, curv.p1.y, x12, y12, x123, y123, x1234, y1234}, tess_tol, level + 1);
        PathBezierCubicCurveToCasteljau(path, BezierCurveSegment2D{x1234, y1234, x234, y234, x34, y34, curv.p4.x, curv.p4.y}, tess_tol, level + 1);
    }
}

static Vec2 BezierCubicCalc(const BezierCurveSegment2D& curve, float t )
{
    float u = 1.0f - t;
    float w1 = u * u * u;
    float w2 = 3 * u * u * t;
    float w3 = 3 * u * t * t;
    float w4 = t * t * t;
    return {
            w1 * curve.p1.x + w2 * curve.p2.x + w3 * curve.p3.x + w4 * curve.p4.x,
            w1 * curve.p1.y + w2 * curve.p2.y + w3 * curve.p3.y + w4 * curve.p4.y
    };
}

std::vector<Vec2>* BezierCurveSegment2D::tesselate(std::vector<Vec2>* path, const BezierCurveSegment2D& curve, int num_segments, float curve_tesselation_tol )
{
    path->push_back(curve.p1);
    if (num_segments == 0)
    {
        ASSERT(curve_tesselation_tol > 0.0f);
        PathBezierCubicCurveToCasteljau(path, curve, curve_tesselation_tol, 0); // Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            path->push_back(BezierCubicCalc(curve, t_step * (float)i_step));
    }
    return path;
}

Rect BezierCurveSegment2D::bbox(const tools::BezierCurveSegment2D& segment )
{
    const std::vector<Vec2> points{ segment.p1, segment.p2, segment.p3, segment.p4};
    return Rect::bounding_rect(&points);
}