#include "routing/pedestrian_directions.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <utility>

namespace
{
bool HasType(uint32_t type, feature::TypesHolder const & types)
{
  for (uint32_t t : types)
  {
    t = ftypes::BaseChecker::PrepareToMatch(t, 2);
    if (type == t)
      return true;
  }
  return false;
}
}  // namespace

namespace routing
{

PedestrianDirectionsEngine::PedestrianDirectionsEngine(std::shared_ptr<NumMwmIds> numMwmIds)
  : m_typeSteps(classif().GetTypeByPath({"highway", "steps"}))
  , m_typeLiftGate(classif().GetTypeByPath({"barrier", "lift_gate"}))
  , m_typeGate(classif().GetTypeByPath({"barrier", "gate"}))
  , m_numMwmIds(std::move(numMwmIds))
{
}

void PedestrianDirectionsEngine::Generate(RoadGraphBase const & graph,
                                          vector<Junction> const & path,
                                          my::Cancellable const & cancellable,
                                          Route::TTimes & times, Route::TTurns & turns,
                                          Route::TStreets & /* streetNames */,
                                          vector<Junction> & routeGeometry,
                                          vector<Segment> & segments)
{
  times.clear();
  turns.clear();
  routeGeometry.clear();
  segments.clear();

  if (path.size() <= 1)
    return;

  CalculateTimes(graph, path, times);

  vector<Edge> routeEdges;
  if (!ReconstructPath(graph, path, routeEdges, cancellable))
  {
    LOG(LDEBUG, ("Couldn't reconstruct path."));
    // use only "arrival" direction
    turns.emplace_back(path.size() - 1, turns::PedestrianDirection::ReachedYourDestination);
    return;
  }

  CalculateTurns(graph, routeEdges, turns, cancellable);

  segments.reserve(routeEdges.size());
  for (Edge const & e : routeEdges)
    segments.push_back(ConvertEdgeToSegment(*m_numMwmIds, e));
  
  routeGeometry = path;
}

void PedestrianDirectionsEngine::CalculateTurns(RoadGraphBase const & graph,
                                                vector<Edge> const & routeEdges,
                                                Route::TTurns & turns,
                                                my::Cancellable const & cancellable) const
{
  for (size_t i = 0; i < routeEdges.size(); ++i)
  {
    if (cancellable.IsCancelled())
      return;

    Edge const & edge = routeEdges[i];

    feature::TypesHolder types;
    graph.GetEdgeTypes(edge, types);

    if (HasType(m_typeSteps, types))
    {
      if (edge.IsForward())
        turns.emplace_back(i, turns::PedestrianDirection::Upstairs);
      else
        turns.emplace_back(i, turns::PedestrianDirection::Downstairs);
    }
    else
    {
      graph.GetJunctionTypes(edge.GetStartJunction(), types);

      if (HasType(m_typeLiftGate, types))
        turns.emplace_back(i, turns::PedestrianDirection::LiftGate);
      else if (HasType(m_typeGate, types))
        turns.emplace_back(i, turns::PedestrianDirection::Gate);
    }
  }

  // direction "arrival"
  // (index of last junction is the same as number of edges)
  turns.emplace_back(routeEdges.size(), turns::PedestrianDirection::ReachedYourDestination);
}
}  // namespace routing
