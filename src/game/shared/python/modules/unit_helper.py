from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class UnitHelper(SemiSharedModuleGenerator):
    module_name = 'unit_helper'

    files = [
        'cbase.h',
        
        'unit_base_shared.h',
        'hl2wars_util_shared.h',
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
        cls.variables('m_pOuter').exclude()
        cls.member_functions('GetOuter').exclude()
        cls.member_functions('GetPyOuter').exclude()
        cls.add_property( 'outer'
                         , cls.member_function('GetPyOuter') )
        #cls.member_functions('GetGroundEntity').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
  
    def AddLocomotion(self, mb):
        mb.free_function('UnitComputePathDirection').include()
        mb.free_function('Unit_ClampYaw').include()
    
        cls = mb.class_('UnitBaseMoveCommand')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.variable('blockers').exclude()
        cls.variable('navignorelist').exclude()
        cls.variable('pyblockers').rename('blockers')
       
        cls = mb.class_('UnitBaseLocomotion')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        
        cls.member_function('HandleJump').virtuality = 'virtual'
        
        cls = mb.class_('UnitAirMoveCommand')
        cls.include() 
        
        # Air locomotion class
        cls = mb.class_('UnitBaseAirLocomotion')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'  
        cls.member_function('UpdateCurrentHeight').virtuality = 'virtual'
        cls.variable('m_fCurrentHeight').rename('currentheight')
        cls.variable('m_fDesiredHeight').rename('desiredheight')
        cls.variable('m_fMaxHeight').rename('maxheight')
        cls.variable('m_fFlyNoiseRate').rename('flynoiserate')
        cls.variable('m_fFlyNoiseZ').rename('flynoisez')
        
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
        cls.variable('m_translateActivityMap').exclude()
        
        cls = mb.class_('UnitAnimConfig')
        cls.include()
        cls.variable('m_flMaxBodyYawDegrees').rename('maxbodyyawdegrees')
        cls.variable('m_bBodyYawNormalized').rename('bodyyawnormalized')
        cls.variable('m_LegAnimType').rename('leganimtype')
        cls.variable('m_bUseAimSequences').rename('useaimsequences')
        cls.variable('m_bInvertPoseParameters').rename('invertposeparameters')
        
        mb.enumeration('LegAnimType_t').include()

        # Combat unit anim state (human like)
        cls = mb.class_('UnitAnimState')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        
        cls.variable('m_pActivityMap').exclude()
        
        cls.member_function('SetAimLayerSequence').exclude()
        cls.member_function('GetAimLayerSequence').exclude()
        cls.add_property( 'aimlayersequence'
                         , cls.member_function('GetAimLayerSequence')
                         , cls.member_function('SetAimLayerSequence') )
        
        cls.member_function('OnEndSpecificActivity').virtuality = 'virtual'
        cls.member_function('OnInterruptSpecificActivity').virtuality = 'virtual'
        
        cls.member_function('GetCustomSpecificActPlaybackRate').exclude()
        cls.member_function('SetCustomSpecificActPlaybackRate').exclude()
        cls.add_property( 'specmainactplaybackrate'
                         , cls.member_function('GetCustomSpecificActPlaybackRate')
                         , cls.member_function('SetCustomSpecificActPlaybackRate') )
        
        cls.variable('m_bPlayFallActInAir').rename('playfallactinair')
        cls.variable('m_fMainPlaybackRate').rename('mainplaybackrate')
        cls.variable('m_iMoveYaw').rename('moveyaw')
        cls.variable('m_iMoveX').rename('movex')
        cls.variable('m_iMoveY').rename('movey')
        cls.variable('m_iBodyYaw').rename('bodyyaw')
        cls.variable('m_iBodyPitch').rename('bodypitch')
        cls.variable('m_iLeanYaw').rename('leanyaw')
        cls.variable('m_iLeanPitch').rename('leanpitch')
        
        cls.variable('m_bFlipMoveY').rename('flipmovey')
        cls.variable('m_bNewJump').rename('newjump')
        cls.variable('m_bJumping').rename('jumping')
        cls.variable('m_flJumpStartTime').rename('jumpstarttime')
        cls.variable('m_bFirstJumpFrame').rename('firstjumpframe')
        
        cls.variable('m_flFeetYawRate').rename('feetyawrate')
        cls.variable('m_flFaceFrontTime').rename('facefronttime')
        
        cls.variable('m_bPlayingMisc').rename('playermisc')
        cls.variable('m_flMiscCycle').rename('misccycle')
        cls.variable('m_flMiscBlendOut').rename('miscblendout')
        cls.variable('m_flMiscBlendIn').rename('miscblendin')
        cls.variable('m_iMiscSequence').rename('miscsequence')
        cls.variable('m_bMiscOnlyWhenStill').rename('misconlywhenstill')
        cls.variable('m_bMiscNoOverride').rename('miscnooverride')
        cls.variable('m_fMiscPlaybackRate').rename('miscplaybackrate')
    
        cls.variable('m_nSpecificMainActivity').rename('specificmainactivity')
        cls.variable('m_fForceAirActEndTime').rename('forceairactendtime')
        
        cls.variable('m_bUseCombatState').rename('usecombatstate')
        cls.variable('m_fCombatStateTime').rename('combatstatetime')
        cls.variable('m_bCombatStateIfEnemy').rename('combatstateifenemy')
        
        # Extensible version of above, with a few more overrible methods (ugly :()
        cls = mb.class_('UnitAnimStateEx')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.member_function('Update').virtuality = 'virtual'
        
        # Vehicle Anim State
        cls = mb.class_('UnitVehicleAnimState')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.member_function('Update').virtuality = 'virtual'
        
        cls.variable('m_iVehicleSteer').rename('vehiclesteer')
        cls.variable('m_iVehicleFLSpin').rename('vehicleflspin')
        cls.variable('m_iVehicleFRSpin').rename('vehiclefrspin')
        cls.variable('m_iVehicleRLSpin').rename('vehiclerlspin')
        cls.variable('m_iVehicleRRSpin').rename('vehiclerrspin')
        cls.variable('m_iVehicleFLHeight').rename('vehicleflheight')
        cls.variable('m_iVehicleFRHeight').rename('vehiclefrheight')
        cls.variable('m_iVehicleRLHeight').rename('vehiclerlheight')
        cls.variable('m_iVehicleRRHeight').rename('vehiclerrheight')
        
        cls.variable('m_fFrontWheelRadius').rename('frontwheelradius')
        cls.variable('m_fRearWheelRadius').rename('rearwheelradius')
        
    def AddNavigator(self, mb):
        # Main class
        cls = mb.class_('UnitBaseNavigator')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        cls.member_functions().exclude()
        cls.member_function('Reset').include()
        cls.member_function('StopMoving').include()
        cls.member_function('Update').include()
        cls.member_function('UpdateIdealAngles').include()
        cls.member_function('SetGoal').include()
        cls.member_function('SetGoalTarget').include()
        cls.member_function('SetGoalInRange').include()
        cls.member_function('SetGoalTargetInRange').include()
        cls.member_function('UpdateGoalInfo').include()
        cls.member_function('GetGoalDistance').include()
        cls.member_function('FindPathAsResult').include()
        cls.member_function('UpdateGoalInRange').include()
        cls.member_function('UpdateGoalTarget').include()
        cls.member_function('DrawDebugRouteOverlay').include()
        cls.member_function('DrawDebugInfo').include()
        
        cls.add_property( 'path'
                         , cls.member_function('PyGetPath')
                         , cls.member_function('SetPath') )
                         
        cls.add_property( 'idealyaw'
                         , cls.member_function('GetIdealYaw')
                         , cls.member_function('SetIdealYaw') )
        cls.add_property( 'facingtarget'
                         , cls.member_function('GetFacingTarget')
                         , cls.member_function('SetFacingTarget') )
        cls.add_property( 'facingtargetpos'
                         , cls.member_function('GetFacingTargetPos')
                         , cls.member_function('SetFacingTargetPos') )
                         
        cls.add_property( 'testroutemask'
                         , cls.member_function('GetTestRouteMask')
                         , cls.member_function('SetTestRouteMask') )
                         
        cls.variable('m_fIdealYawTolerance').rename('idealyawtolerance') 
        cls.variable('m_fFacingCone').rename('facingcone') 
        cls.variable('m_bFacingFaceTarget').rename('facingfacetarget') 
        
        cls.variable('m_vForceGoalVelocity').rename('forcegoalvelocity')
        
        cls.variable('m_bNoAvoid').rename('noavoid')
        cls.variable('m_bNoPathVelocity').rename('nopathvelocity')
        cls.variable('m_bNoSlowDownToTarget').rename('no_slow_down_to_target')
        
        # Path class
        cls = mb.class_('UnitBasePath')
        cls.include()
        cls.member_function('GetCurWaypoint').exclude()
        cls.member_function('SetWaypoint').exclude()
        cls.variable('m_iGoalType').rename('goaltype')
        cls.variable('m_pWaypointHead').exclude()
        cls.variable('m_vGoalPos').rename('goalpos')
        cls.variable('m_waypointTolerance').rename('waypointtolerance')
        cls.variable('m_fGoalTolerance').rename('goaltolerance')
        cls.variable('m_iGoalFlags').rename('goalflags')
        cls.variable('m_fMinRange').rename('minrange')
        cls.variable('m_fMaxRange').rename('maxrange')
        cls.variable('m_hTarget').exclude()
        cls.variable('m_bAvoidEnemies').rename('avoidenemies')
        cls.variable('m_fMaxMoveDist').rename('maxmovedist')
        cls.variable('m_vStartPosition').rename('startposition')
        cls.variable('m_bSuccess').rename('success')
        cls.variable('m_fnCustomLOSCheck').rename('fncustomloscheck')
        cls.variable('m_pathContext').rename('pathcontext')
        
        cls.member_function('GetTarget').exclude()
        cls.member_function('SetTarget').exclude()
        cls.add_property( 'target'
                         , cls.member_function('GetTarget')
                         , cls.member_function('SetTarget') )
        
        
        mb.enumeration('UnitGoalFlags').include()
        mb.enumeration('UnitGoalTypes').include()
        
        # Air Unit version
        cls = mb.class_('UnitBaseAirNavigator')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        cls.member_functions().exclude()
        cls.member_function('Update').include()
        cls.add_property( 'testrouteworldonly'
                         , cls.member_function('GetTestRouteWorldOnly')
                         , cls.member_function('SetTestRouteWorldOnly') )
        cls.add_property( 'usesimplifiedroutebuilding'
                         , cls.member_function('GetUseSimplifiedRouteBuilding')
                         , cls.member_function('SetUseSimplifiedRouteBuilding') )
                         
        # Vehicle Unit version
        cls = mb.class_('UnitVehicleNavigator')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        cls.member_functions().exclude()
        cls.member_function('UpdateIdealAngles').include()

    def AddSense(self, mb):
        cls = mb.class_('UnitBaseSense')
        cls.include()
        cls.calldefs().virtuality = 'not virtual'
        
        cls.member_functions('GetEnemy').exclude()
        cls.member_functions('GetOther').exclude()
        cls.member_functions('PyGetEnemy').rename('GetEnemy')
        cls.member_functions('PyGetOther').rename('GetOther')
        cls.member_functions('PyGetEnemies').rename('GetEnemies')
        cls.member_functions('PyGetOthers').rename('GetOthers')
        
        cls.variables('m_fSenseDistance').rename('sensedistance')
        cls.variables('m_fSenseRate').rename('senserate')
        
        cls.member_functions('GetTestLOS').exclude()
        cls.member_functions('SetTestLOS').exclude()
        cls.add_property( 'testlos'
                         , cls.member_function('GetTestLOS')
                         , cls.member_function('SetTestLOS') )
        
    def AddAnimEventMap(self, mb):
        cls = mb.class_('AnimEventMap')
        cls.include()
        
        cls = mb.class_('BaseAnimEventHandler')
        cls.include()

        cls = mb.class_('EmitSoundAnimEventHandler')
        cls.include()
        cls.member_functions().virtuality = 'not virtual' 
        
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
        
        cls.member_function('GetMySpeechSemaphore').exclude()
        cls.member_functions('GetOuter').exclude()
        cls.member_function('GetSink').exclude()
        if self.settings.branch  not in {'swarm', 'lambdawars'}:
            cls.member_function('SpeakFindResponse').exclude()
        
        cls = mb.class_('UnitExpresser')
        cls.include()
        
    def AddMisc(self, mb):
        mb.free_function('VecCheckThrowTolerance').include()
        
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
            self.AddSense(mb)
            self.AddAnimEventMap(mb)
            self.AddExpresser(mb)
        self.AddMisc(mb)
        
        # Finally apply common rules to all includes functions and classes, etc.
        self.ApplyCommonRules(mb)
            