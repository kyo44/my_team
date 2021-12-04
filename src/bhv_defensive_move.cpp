// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#include <list>
#endif

#include "bhv_basic_move.h"
#include "bhv_defensive_move.h"

#include "strategy.h"

#include "bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/soccer_intention.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include "neck_offensive_intercept_neck.h"

using namespace rcsc;

class IntentionDefence
    : public SoccerIntention {
private:
    const Vector2D M_target_point; //!< trapped ball position
    int M_step;
public:
    IntentionDefence( const Vector2D & target_point,
                      const int n_step )
      : M_target_point( target_point ),
        M_step( n_step )
    { }

    bool finished( const PlayerAgent * agent );
    bool execute( PlayerAgent * agent );
private:
  void clear()
  {
    M_step = 0;
  }
};
/*-------------------------------------------------------------------*/
/*!

 */
bool
IntentionDefence::finished( const PlayerAgent * agent)
{
  const WorldModel & wm = agent->world();
  if ( M_step == 0 )
  {
    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (finished) check finished. empty queue" );
    return true;
  }
  if ( wm.ball().posCount() == 4 )
  {
    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (finished) don't look ball" );
    return true;
  }
  //const WorldModel & wm = agent->world();

  //if ( wm.existKickableOpponent() || wm.self().isKickable() )
  //{
  //  dlog.addText( Logger::TEAM,
  //                __FILE__":(finished). opponent has ball now" );
  //  return true;
  //}
  return false;
}

bool
IntentionDefence::execute( PlayerAgent * agent )
{
  dlog.addText( Logger::DRIBBLE,
                __FILE__": in IntentionDefence" );
  const WorldModel & wm = agent->world();
  //if ( wm.existKickableOpponent() )
  //{
  //  dlog.addText( Logger::DRIBBLE,
  //                __FILE__": (finished) opponent has ball" );
  //  this->clear();
  //  return false;
  //}
  if ( wm.getTeammateNearestToBall(30,false) == NULL)
  {
    dlog.addText( Logger::DRIBBLE,
                  __FILE__": (finished) nearest teammate don't know" );
    this->clear();
    return false;
  }
  //wm.getTeammateNearestToBall(30,false)->unum() == 2
  //     &&
  if ( wm.self().distFromBall() > 10.0 )
    {
      --M_step;
      dlog.addText( Logger::DRIBBLE,
                    __FILE__": ball has 2, i move(0.0, 0.0)" );
      dlog.addText( Logger::DRIBBLE,
                    __FILE__": M_step = %d", M_step );
      agent->debugClient().setTarget( M_target_point );
      if ( M_step > 50-5 )
      {
        dlog.addText( Logger::DRIBBLE,
                      __FILE__": Turn now" );
        Body_TurnToPoint( M_target_point, 1 ).execute( agent );
        return true;
      }
      dlog.addText( Logger::DRIBBLE,
                    __FILE__": dash" );
      agent->doDash( ServerParam::i().maxDashPower() );
      return true;
    }
  return false;
}
/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_DefensiveMove::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_DefensiveMove" );

    //-----------------------------------------------
    // tackle
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {
        return true;
    }

    const WorldModel & wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept" );
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

        return true;
    }




    //my chenge code start
    //intention
    const Vector2D t_point(52.5, 0.0);
    agent->setIntention( new IntentionDefence( t_point ,
                                               50 ) );

    //move
    bool mark = false;		//if important-move-player is mark-true
    bool L_side = true;		//if Left-side is true
    double center = 0.75;	//if center-position is 0.75, side-posirion is 0.25
    Vector2D target_point;
    //position check
    if ( wm.self().unum() == 3 )
    {
	L_side = false;
	dlog.addText( Logger::TEAM,
                      __FILE__": unum = %d(3)", wm.self().unum());
    }
    else if ( wm.self().unum() == 4 )
    {
	center = 0.25;
	dlog.addText( Logger::TEAM,
		      __FILE__": unum = %d(4)", wm.self().unum());
    }
    else if ( wm.self().unum() == 5 )
    {
	L_side = false;
	center = 0.25;
	dlog.addText( Logger::TEAM,
		      __FILE__": unum = %d(5)", wm.self().unum());
    }
    else
    {
	dlog.addText( Logger::TEAM,
		      __FILE__": unum = %d(2)", wm.self().unum());
    }

    //my-side check and move-change
    if ( (wm.ball().pos().y < 0 && L_side == true) ||
	 (wm.ball().pos().y > 0 && L_side == false) )
    {
	if ( center == 0.25)
        {
            mark = true;
        }
        dlog.addText( Logger::TEAM,
      	              __FILE__": my-side & do defensive_move");
        //area in goal line
        if ( wm.ball().pos().absX() > 35.5 )
        {
	    dlog.addText( Logger::TEAM,
		          __FILE__": in goal-line-area");
            //side
	    if ( wm.ball().pos().absY() > 7 )
            {
		//if side-back don't be, chenge position center-back and side-back
                //side-back-player
		if ( center == 0.25 && wm.ball().pos().absX() - 5.0  > wm.self().pos().absX() )
		{
                    mark = false;
                    center = 0.75;
                    dlog.addText( Logger::TEAM,
				  __FILE__": move center-back-position" );
		}
                //center-back-player
                else if ( center == 0.75 && wm.ourPlayer( wm.self().unum() + 2 ) != NULL )
                {
                    if ( wm.ball().pos().absX() - 5.0 >
                         wm.ourPlayer( wm.self().unum() + 2 )->pos().absX() )
                    {
                        mark = true;
                        center = 0.25;
                        dlog.addText( Logger::TEAM,
                                      __FILE__": move side-back-position" );
                    }
                }
                target_point.x = wm.ball().pos().absX();
                target_point.y = wm.ball().pos().absY() +
                                 ( 7.0 - wm.ball().pos().absY() ) * center;
            }
            //center
            else
            {
                target_point.x = wm.ball().pos().absX() - 1.0;
                target_point.y = wm.ball().pos().absY();
            }
            //chenge plus-minus
            if ( wm.ball().pos().x < 0 )
            {
                target_point.x *= -1;
	    }
            if ( wm.ball().pos().y < 0 )
            {
                target_point.y *= -1;
            }
        }
	//area in penalty area
	else if (wm.ball().pos().absX() > 20)
	{
	    dlog.addText( Logger::TEAM,
			  __FILE__": in penalty-line-area");
	    target_point = Strategy::i().getPosition( wm.self().unum() );
	}
	else
	{
	    target_point = Strategy::i().getPosition( wm.self().unum() );
	}
    }
    //if not my-side move-basic
    else
    {
        target_point = Strategy::i().getPosition( wm.self().unum() );
	dlog.addText( Logger::TEAM,
		      __FILE__": not my-side & do basic_move");
    }


    //if not mark-player move nearly-opp-player
    if ( mark == false )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": nearly-opp-player mark");
        //check free-opp-player
        AbstractPlayerCont::const_iterator fin = wm.ourPlayers().end();
        std::list<Vector2D> mark_f;
        for ( AbstractPlayerCont::const_iterator p = wm.ourPlayers().begin();
              p != fin; ++p )
        {
            if ( ! (*p) ) continue;
            mark_f.push_back( (*p)->pos() );
            dlog.addText( Logger::TEAM, __FILE__": %d:mikata", (*p)->pos());
        }
        dlog.addText( Logger::TEAM, __FILE__": %d:mikata", mark_f);
    }
    //my change code finish




    dlog.addText( Logger::TEAM,
                  __FILE__": target_point = (%.1f, %.1f)", target_point.x, target_point.y );
    const double dash_power = Strategy::get_normal_dash_power( wm );

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_DefensiveMove target=(%.1f %.1f) dist_thr=%.2f",
                  target_point.x, target_point.y,
                  dist_thr );

    agent->debugClient().addMessage( "DefensiveMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    if ( wm.existKickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan() );
    }

    return true;
}
