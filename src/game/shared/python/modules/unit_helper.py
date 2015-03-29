from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class UnitHelper(SemiSharedModuleGenerator):
    module_name = 'unit_helper'

    files = [
        'cbase.h',
        
        'unit_base_shared.h',
        'unit_component.h',
        'unit_locomotion.h',
        'unit_airlocomotion.h',
        'unit_vphysicslocomotion.h',
        'unit_animstate.h',
        'unit_vehicleanimstate.h',
        #'wars_orders.h'
        
        '$c_baseplayer.h',
        '$c_unit_base.h',
        
        '#player.h',
        '#unit_expresser.h',
        '#unit_navigator.h',
        '#unit_airnavigator.h',
        '#unit_intention.h',
        '#unit_sense.h',
        '#unit_base.h',
        '#unit_vehiclenavigator.h',
        'wars_func_unit.h',
    ]
    
    def AddUnitComponent(self, mb):
        cls = mb.class_('UnitComponent')
        cls.include()
        cls.vars('m_pOuter').exclude()
        cls.mem_funs('GetOuter').exclude()
        cls.mem_funs('GetPyOuter').exclude()
        cls.add_property( 'outer'
                         , cls.mem_fun('GetPyOuter') )
        #cls.mem_funs('GetGroundEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
  
    def AddLocomotion(self, mb):
        mb.free_function('UnitComputePathDirection').include()
        mb.free_function('Unit_ClampYaw').include()
    
        cls = mb.class_('UnitBaseMoveCommand')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.var('blockers').exclude()
        cls.var('navignorelist').exclude()
        cls.var('pyblockers').rename('blockers')
       
        cls = mb.class_('UnitBaseLocomotion')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        
        cls.mem_fun('HandleJump').virtuality = 'virtual'
        
        cls = mb.class_('UnitAirMoveCommand')
        cls.include() 
        
        # Air locomotion class
        cls = mb.class_('UnitBaseAirLocomotion')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'  
        cls.mem_fun('UpdateCurrentHeight').virtuality = 'virtual'
        cls.var('m_fCurrentHeight').rename('currentheight')
        cls.var('m_fDesiredHeight').rename('desiredheight')
        cls.var('m_fMaxHeight').rename('maxheight')
        cls.var('m_fFlyNoiseRate').rename('flynoiserate')
        cls.var('m_fFlyNoiseZ').rename('flynoisez')
        
        # VPhysicsLocomotion
        cls = mb.class_('UnitVPhysicsLocomotion')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'  

    def AddAnimState(self, mb):
        # Base Animation State
        cls = mb.class_('UnitBaseAnimState')
        cls.include()
        
        # Specific Optimized Unit Animation State
        cls = mb.class_('TranslateActivityMap')
        cls.include()
        cls.no_init = False
        cls.var('m_translateActivityMap').exclude()
        
        cls = mb.class_('UnitAnimConfig')
        cls.include()
        cls.var('m_flMaxBodyYawDegrees').rename('maxbodyyawdegrees')
        cls.var('m_bBodyYawNormalized').rename('bodyyawnormalized')
        cls.var('m_LegAnimType').rename('leganimtype')
        cls.var('m_bUseAimSequences').rename('useaimsequences')
        cls.var('m_bInvertPoseParameters').rename('invertposeparameters')
        
        mb.enum('LegAnimType_t').include()

        # Combat unit anim state (human like)
        cls = mb.class_('UnitAnimState')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        
        cls.var('m_pActivityMap').exclude()
        
        cls.mem_fun('SetAimLayerSequence').exclude()
        cls.mem_fun('GetAimLayerSequence').exclude()
        cls.add_property( 'aimlayersequence'
                         , cls.mem_fun('GetAimLayerSequence')
                         , cls.mem_fun('SetAimLayerSequence') )
        
        cls.mem_fun('OnEndSpecificActivity').virtuality = 'virtual'
        
        cls.mem_fun('GetCustomSpecificActPlaybackRate').exclude()
        cls.mem_fun('SetCustomSpecificActPlaybackRate').exclude()
        cls.add_property( 'specmainactplaybackrate'
                         , cls.mem_fun('GetCustomSpecificActPlaybackRate')
                         , cls.mem_fun('SetCustomSpecificActPlaybackRate') )
        
        cls.var('m_bPlayFallActInAir').rename('playfallactinair')
        cls.var('m_fMainPlaybackRate').rename('mainplaybackrate')
        cls.var('m_iMoveYaw').rename('moveyaw')
        cls.var('m_iMoveX').rename('movex')
        cls.var('m_iMoveY').rename('movey')
        cls.var('m_iBodyYaw').rename('bodyyaw')
        cls.var('m_iBodyPitch').rename('bodypitch')
        cls.var('m_iLeanYaw').rename('leanyaw')
        cls.var('m_iLeanPitch').rename('leanpitch')
        
        cls.var('m_bFlipMoveY').rename('flipmovey')
        cls.var('m_bNewJump').rename('newjump')
        cls.var('m_bJumping').rename('jumping')
        cls.var('m_flJumpStartTime').rename('jumpstarttime')
        cls.var('m_bFirstJumpFrame').rename('firstjumpframe')
        
        cls.var('m_flFeetYawRate').rename('feetyawrate')
        cls.var('m_flFaceFrontTime').rename('facefronttime')
        
        cls.var('m_bPlayingMisc').rename('playermisc')
        cls.var('m_flMiscCycle').rename('misccycle')
        cls.var('m_flMiscBlendOut').rename('miscblendout')
        cls.var('m_flMiscBlendIn').rename('miscblendin')
        cls.var('m_iMiscSequence').rename('miscsequence')
        cls.var('m_bMiscOnlyWhenStill').rename('misconlywhenstill')
        cls.var('m_bMiscNoOverride').rename('miscnooverride')
        cls.var('m_fMiscPlaybackRate').rename('miscplaybackrate')
    
        cls.var('m_nSpecificMainActivity').rename('specificmainactivity')
        cls.var('m_fForceAirActEndTime').rename('forceairactendtime')
        
        cls.var('m_bUseCombatState').rename('usecombatstate')
        cls.var('m_fCombatStateTime').rename('combatstatetime')
        cls.var('m_bCombatStateIfEnemy').rename('combatstateifenemy')
        
        # Extensible version of above, with a few more overrible methods (ugly :()
        cls = mb.class_('UnitAnimStateEx')
        cls.include()
        cls.mem_fun('Update').virtuality = 'virtual'
        
        # Vehicle Anim State
        cls = mb.class_('UnitVehicleAnimState')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.mem_fun('Update').virtuality = 'virtual'
        
        cls.var('m_iVehicleSteer').rename('vehiclesteer')
        cls.var('m_iVehicleFLSpin').rename('vehicleflspin')
        cls.var('m_iVehicleFRSpin').rename('vehiclefrspin')
        cls.var('m_iVehicleRLSpin').rename('vehiclerlspin')
        cls.var('m_iVehicleRRSpin').rename('vehiclerrspin')
        cls.var('m_iVehicleFLHeight').rename('vehicleflheight')
        cls.var('m_iVehicleFRHeight').rename('vehiclefrheight')
        cls.var('m_iVehicleRLHeight').rename('vehiclerlheight')
        cls.var('m_iVehicleRRHeight').rename('vehiclerrheight')
        
        cls.var('m_fFrontWheelRadius').rename('frontwheelradius')
        cls.var('m_fRearWheelRadius').rename('rearwheelradius')
        
    def AddNavigator(self, mb):
        # Main class
        cls = mb.class_('UnitBaseNavigator')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        cls.mem_funs().exclude()
        cls.mem_fun('Reset').include()
        cls.mem_fun('StopMoving').include()
        cls.mem_fun('Update').include()
        cls.mem_fun('UpdateIdealAngles').include()
        cls.mem_fun('RegenerateConsiderList').include()
        cls.mem_fun('CalcMove').include()
        cls.mem_fun('SetGoal').include()
        cls.mem_fun('SetGoalTarget').include()
        cls.mem_fun('SetGoalInRange').include()
        cls.mem_fun('SetGoalTargetInRange').include()
        #cls.mem_fun('SetVectorGoal').include()
        cls.mem_fun('FindPathAsResult').include()
        cls.mem_fun('UpdateGoalInRange').include()
        cls.mem_fun('UpdateGoalTarget').include()
        cls.mem_fun('GetGoalDistance').include()
        cls.mem_fun('LimitPosition').include()
        cls.mem_fun('ClearLimitPosition').include()
        cls.mem_fun('DrawDebugRouteOverlay').include()
        cls.mem_fun('DrawDebugInfo').include()
        
        cls.add_property( 'path'
                         , cls.mem_fun('PyGetPath')
                         , cls.mem_fun('SetPath') )
                         
        cls.add_property( 'idealyaw'
                         , cls.mem_fun('GetIdealYaw')
                         , cls.mem_fun('SetIdealYaw') )
        cls.add_property( 'facingtarget'
                         , cls.mem_fun('GetFacingTarget')
                         , cls.mem_fun('SetFacingTarget') )
        cls.add_property( 'facingtargetpos'
                         , cls.mem_fun('GetFacingTargetPos')
                         , cls.mem_fun('SetFacingTargetPos') )
                         
        cls.add_property( 'testroutemask'
                         , cls.mem_fun('GetTestRouteMask')
                         , cls.mem_fun('SetTestRouteMask') )
                         
        cls.var('m_fIdealYawTolerance').rename('idealyawtolerance') 
        cls.var('m_fFacingCone').rename('facingcone') 
        cls.var('m_bFacingFaceTarget').rename('facingfacetarget') 
        
        cls.var('m_vForceGoalVelocity').rename('forcegoalvelocity')
        
        cls.var('m_bNoAvoid').rename('noavoid')
        cls.var('m_bNoPathVelocity').rename('nopathvelocity')
        
        # Path class
        cls = mb.class_('UnitBasePath')
        cls.include()
        cls.mem_fun('GetCurWaypoint').exclude()
        cls.mem_fun('SetWaypoint').exclude()
        cls.var('m_iGoalType').rename('goaltype')
        cls.var('m_pWaypointHead').exclude()
        cls.var('m_vGoalPos').rename('goalpos')
        cls.var('m_waypointTolerance').rename('waypointtolerance')
        cls.var('m_fGoalTolerance').rename('goaltolerance')
        cls.var('m_iGoalFlags').rename('goalflags')
        cls.var('m_fMinRange').rename('minrange')
        cls.var('m_fMaxRange').rename('maxrange')
        cls.var('m_hTarget').exclude()
        cls.var('m_bAvoidEnemies').rename('avoidenemies')
        cls.var('m_fMaxMoveDist').rename('maxmovedist')
        cls.var('m_vStartPosition').rename('startposition')
        cls.var('m_bSuccess').rename('success')
        cls.var('m_fnCustomLOSCheck').rename('fncustomloscheck')
        cls.var('m_pathContext').rename('pathcontext')
        
        cls.mem_fun('GetTarget').exclude()
        cls.mem_fun('SetTarget').exclude()
        cls.add_property( 'target'
                         , cls.mem_fun('GetTarget')
                         , cls.mem_fun('SetTarget') )
        
        
        mb.enum('UnitGoalFlags').include()
        mb.enum('UnitGoalTypes').include()
        
        # Air Unit version
        cls = mb.class_('UnitBaseAirNavigator')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        cls.mem_funs().exclude()
        cls.mem_fun('Update').include()
        cls.add_property( 'testrouteworldonly'
                         , cls.mem_fun('GetTestRouteWorldOnly')
                         , cls.mem_fun('SetTestRouteWorldOnly') )
        cls.add_property( 'usesimplifiedroutebuilding'
                         , cls.mem_fun('GetUseSimplifiedRouteBuilding')
                         , cls.mem_fun('SetUseSimplifiedRouteBuilding') )
                         
        # Vehicle Unit version
        cls = mb.class_('UnitVehicleNavigator')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
                         
    def AddIntention(self, mb):
        pass
        # BaseAction
        #cls = mb.class_('BaseAction')
        #cls.include()

        #mb.add_registration_code( "bp::scope().attr( \"CONTINUE\" ) = CONTINUE;" )
        #mb.add_registration_code( "bp::scope().attr( \"CHANGETO\" ) = CHANGETO;" )
        #mb.add_registration_code( "bp::scope().attr( \"SUSPENDFOR\" ) = SUSPENDFOR;" )
        #mb.add_registration_code( "bp::scope().attr( \"DONE\" ) = DONE;" )
        
    def AddSense(self, mb):
        cls = mb.class_('UnitBaseSense')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        
        cls.mem_funs('GetEnemy').exclude()
        cls.mem_funs('GetOther').exclude()
        cls.mem_funs('PyGetEnemy').rename('GetEnemy')
        cls.mem_funs('PyGetOther').rename('GetOther')
        cls.mem_funs('PyGetEnemies').rename('GetEnemies')
        cls.mem_funs('PyGetOthers').rename('GetOthers')
        
        cls.vars('m_fSenseDistance').rename('sensedistance')
        cls.vars('m_fSenseRate').rename('senserate')
        
        cls.mem_funs('GetTestLOS').exclude()
        cls.mem_funs('SetTestLOS').exclude()
        cls.add_property( 'testlos'
                         , cls.mem_fun('GetTestLOS')
                         , cls.mem_fun('SetTestLOS') )
        
    def AddAnimEventMap(self, mb):
        cls = mb.class_('AnimEventMap')
        cls.include()
        
        cls = mb.class_('BaseAnimEventHandler')
        cls.include()

        cls = mb.class_('EmitSoundAnimEventHandler')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        
        cls = mb.class_('TossGrenadeAnimEventHandler')
        cls.include()
        
    def AddExpresser(self, mb):
        if self.settings.branch == 'swarm':
            cls = mb.class_('CriteriaSet')
        else:
            cls = mb.class_('AI_CriteriaSet')
        cls.include()
    
        cls = mb.class_('CAI_Expresser')
        cls.include()
        
        cls.mem_fun('GetMySpeechSemaphore').exclude()
        cls.mem_funs('GetOuter').exclude()
        cls.mem_fun('GetSink').exclude()
        if self.settings.branch != 'swarm':
            cls.mem_fun('SpeakFindResponse').exclude()
        
        cls = mb.class_('UnitExpresser')
        cls.include()
        
    def AddMisc(self, mb):
        if self.isserver:
            mb.free_function('VecCheckThrowTolerance').include()
            #if self.isclient:
            #    mb.free_function('DrawHealthBar').include()
        
        '''cls = mb.class_('GroupMoveOrder')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.mem_funs().exclude()
        cls.class_('UnitRatio_t').exclude()
        cls.class_('PositionRatio_t').exclude()
        cls.mem_funs('Apply').include()
        cls.mem_funs('AddUnit').include()'''
        
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude() 

        # Components
        self.AddUnitComponent(mb)
        self.AddLocomotion(mb)
        self.AddAnimState(mb)
        if self.isserver:
            self.AddNavigator(mb)
            self.AddIntention(mb)
            self.AddSense(mb)
            self.AddAnimEventMap(mb)
            self.AddExpresser(mb)
        self.AddMisc(mb)
        
        # Finally apply common rules to all includes functions and classes, etc.
        self.ApplyCommonRules(mb)
            