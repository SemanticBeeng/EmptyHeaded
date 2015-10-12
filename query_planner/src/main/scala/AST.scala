package DunceCap

import argonaut.Json

abstract trait ASTStatement {
}

//input to this should be 
//(1) list of attrs in the output, 
//(2) list of attrs eliminated (aggregations), 
//(3) list of relations joined
//(4) list of attrs with selections
//(5) list of exressions for aggregations
class ASTLambdaFunction(val inputArgument:QueryRelation,
                        val join:List[QueryRelation],
                        val aggregates:Map[String,ParsedAggregate])

//change this to lhs; aggregates[attr,(op,expression,init)], join, recursion
case class ASTQueryStatement(
                              lhs:QueryRelation,
                              joinType:String,
                              join:List[QueryRelation],
                              recursion:Option[RecursionStatement],
                              tc:Option[TransitiveClosureStatement],
                              joinAggregates:Map[String,ParsedAggregate]) extends ASTStatement {
  // TODO (sctu) : ignoring everything except for join, joinAggregates for now

  var queryPlan: Json = getGHD()

  private def getGHD(): Json = {
    // We get the candidate GHDs, i.e., the ones of min width
    val rootNodes = GHDSolver.getMinFHWDecompositions(join);
    val candidates = rootNodes.map(r => new GHD(r, join, joinAggregates, lhs));
    candidates.map(c => c.doPostProcessingPass())
    HeuristicUtils.getGHDsWithMaxCoveringRoot(
      HeuristicUtils.getGHDsWithMinBags(candidates))
    val queryPlan = candidates.head.toJson
    return queryPlan
  }

  override def toString(): String = {
    queryPlan.toString
  }
}