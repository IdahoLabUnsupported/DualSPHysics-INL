//HEAD_DSPH
/*The revision belongs to Copyright 2024, Battelle Energy Alliance, LLC All Rights Reserved*/
/*
 <DUALSPHYSICS>  Copyright (c) 2020 by Dr Jose M. Dominguez et al. (see http://dual.sphysics.org/index.php/developers/). 

 EPHYSLAB Environmental Physics Laboratory, Universidade de Vigo, Ourense, Spain.
 School of Mechanical, Aerospace and Civil Engineering, University of Manchester, Manchester, U.K.

 This file is part of DualSPHysics. 

 DualSPHysics is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License 
 as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
 
 DualSPHysics is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details. 

 You should have received a copy of the GNU Lesser General Public License along with DualSPHysics. If not, see <http://www.gnu.org/licenses/>. 
*/

//:NO_COMENTARIO
//:#############################################################################
//:# Cambios:
//:# =========
//:# - Gestiona la inizializacion inicial de las particulas (01-02-2017)
//:# - Implementa normales para cilindors (body & tank) y esfera (tank). (31-01-2020)
//:# - Limita calculo de normales con MaxDisteH. (31-01-2020)
//:# - Objeto JXml pasado como const para operaciones de lectura. (18-03-2020)  
//:# - Comprueba opcion active en elementos de primer y segundo nivel. (19-03-2020)  
//:# - Opcion para calcular boundary limit de forma automatica. (19-05-2020)  
//:# - Cambio de nombre de J.SphInitialize a J.DsInitialize. (28-06-2020)
//:# - Error corregido al obtener nombre de operacion a partir de la clase. (02-07-2020)
//:# - New filter onlypos according to particle position. (25-07-2020)
//:#############################################################################

/// \file JDsInitialize.h \brief Declares the class \ref JDsInitialize.

#ifndef _JDsInitialize_
#define _JDsInitialize_

#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "JObject.h"
#include "TypesDef.h"

class JXml;
class TiXmlElement;

//##############################################################################
//# XML format in _FmtXML_Initialize.xml.
//##############################################################################

//##############################################################################
//# JDsInitializeOp
//##############################################################################
/// \brief Base clase for initialization of particle data.
class JDsInitializeOp : public JObject
{
public:
  ///Types of initializations.
  typedef enum{ 
    IT_FluidVel=10
   ,IT_BoundNormalSet=30
   ,IT_BoundNormalPlane=31
   ,IT_BoundNormalSphere=32
   ,IT_BoundNormalCylinder=33
   ,IT_BoundNormalCurved=34,
  }TpInitialize; 

  ///Structure with constant values needed for initialization tasks.
  typedef struct StrInitCt{
    float kernelh;      ///<The smoothing length of SPH kernel [m].
    float dp;           ///<Initial distance between particles [m].
    unsigned nbound;    ///<Initial number of boundary particles (fixed+moving+floating).
    std::string dirdatafile; ///<Directory to data files.
    StrInitCt(float kernelh_,float dp_,unsigned nbound_,std::string dirdatafile_){
      kernelh=kernelh_; dp=dp_; nbound=nbound_; dirdatafile=dirdatafile_;
    }
  }StInitCt;

protected:
  bool OnlyPos;         ///<Activate filter according to position.
  tdouble3 OnlyPosMin;  ///<Minimum positon for filtering.
  tdouble3 OnlyPosMax;  ///<Maximum positon for filtering.
  unsigned NpUpdated;   ///<Number of updated particles.
  unsigned NpTotal;     ///<Total number of particles.
public:
  const TpInitialize Type;   ///<Type of particle.
  const StInitCt InitCt;     ///<Constant values needed for initialization tasks.
  const unsigned BaseNameSize;

public:
  JDsInitializeOp(TpInitialize type,const char* name,StInitCt initct)
    :Type(type),InitCt(initct),BaseNameSize(unsigned(std::string("JDsInitializeOp_").size()))
  { 
    ClassName=std::string("JDsInitializeOp_")+name;
    Reset();
  } 
  virtual ~JDsInitializeOp(){ DestructorActive=true; }
  void Reset();
  void ReadXmlOnlyPos(const JXml *sxml,TiXmlElement* ele);
  virtual void ReadXml(const JXml *sxml,TiXmlElement* ele)=0;
  virtual void Run(unsigned np,unsigned npb,const tdouble3 *pos
    ,const unsigned *idp,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner)=0;
  virtual void GetConfig(std::vector<std::string> &lines)const=0;
  unsigned ComputeDomainMk(bool bound,word mktp,unsigned np,const word *mktype
    ,const unsigned *idp,const tdouble3 *pos,tdouble3 &posmin,tdouble3 &posmax)const;
  inline bool CheckPos(unsigned p,const tdouble3 *pos){
    NpTotal++;
    const bool sel=(!OnlyPos || (OnlyPosMin<=pos[p] && pos[p]<=OnlyPosMax));
    if(sel)NpUpdated++;
    return(sel);
  }
  std::string GetConfigNp()const;
  std::string GetConfigMkBound(std::string mktype)const;
  std::string GetConfigMkFluid(std::string mktype)const;
  std::string GetConfigOnlyPos()const;
};

//##############################################################################
//# JDsInitializeOp_FluidVel
//##############################################################################
/// Initializes velocity of fluid particles.
class JDsInitializeOp_FluidVel : public JDsInitializeOp
{
private:
  ///Controls profile of imposed velocity.
  typedef enum{ 
    TVEL_Constant=0   ///<Velocity profile uniform.
   ,TVEL_Linear=1     ///<Velocity profile linear.
   ,TVEL_Parabolic=2  ///<Velocity profile parabolic.
  }TpVelocity;
private:
  TpVelocity VelType;  ///<Type of velocity.
  std::string MkFluid;
  tfloat3 Direction;
  float Vel1,Vel2,Vel3;
  float Posz1,Posz2,Posz3;
public:
  JDsInitializeOp_FluidVel(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JDsInitializeOp(IT_FluidVel,"FluidVel",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset();
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JDsInitializeOp_BoundNormalCurved
//##############################################################################
/// Initializes normals of boundary particles.
class JDsInitializeOp_BoundNormalCurved : public JDsInitializeOp
{
private:
    std::string MkBound;
    tfloat3 Center1;
    tfloat3 Center2;
    bool Inverse;
    bool Corner;      
public:
    JDsInitializeOp_BoundNormalCurved(const JXml* sxml, TiXmlElement* ele, StInitCt initct)
        :JDsInitializeOp(IT_BoundNormalCurved, "BoundNormalCurved", initct) {
        Reset(); ReadXml(sxml, ele);
    }
    void Reset();
    void ReadXml(const JXml* sxml, TiXmlElement* ele);
    void Run(unsigned np, unsigned npb, const tdouble3* pos, const unsigned* idp
        , const word* mktype, tfloat4* velrhop, tfloat3* boundnormal,bool *BoundCorner);
    void GetConfig(std::vector<std::string>& lines)const;
};

//##############################################################################
//# JDsInitializeOp_BoundNormalSet
//##############################################################################
/// Initializes normals of boundary particles.
class JDsInitializeOp_BoundNormalSet : public JDsInitializeOp
{
private:
  std::string MkBound;
  tfloat3 Normal;
  bool Corner;
public:
  JDsInitializeOp_BoundNormalSet(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JDsInitializeOp(IT_BoundNormalSet,"BoundNormalSet",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset();
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JDsInitializeOp_BoundNormalPlane
//##############################################################################
/// Initializes normals of boundary particles.
class JDsInitializeOp_BoundNormalPlane : public JDsInitializeOp
{
private:
  std::string MkBound;
  bool PointAuto;   ///<Point is calculated automatically accoding to normal configuration.
  float LimitDist;  ///<Minimun distance (Dp*vdp) between particles and boundary limit to calculate the point (default=0.5).
  tfloat3 Point;
  tfloat3 Normal;
  bool Corner;
  float MaxDisteH;  ///<Maximum distance to boundary limit. It uses H*distanceh (default=2).
public:
  JDsInitializeOp_BoundNormalPlane(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JDsInitializeOp(IT_BoundNormalPlane,"BoundNormalPlane",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset();
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JDsInitializeOp_BoundNormalSphere
//##############################################################################
/// Initializes normals of boundary particles.
class JDsInitializeOp_BoundNormalSphere : public JDsInitializeOp
{
private:
  std::string MkBound;
  tfloat3 Center;
  float Radius;
  bool Inside;      ///<Boundary particles inside the sphere.
  bool Corner;
  float MaxDisteH;  ///<Maximum distance to boundary limit. It uses H*distanceh (default=2).
public:
  JDsInitializeOp_BoundNormalSphere(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JDsInitializeOp(IT_BoundNormalSphere,"BoundNormalSphere",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset();
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JDsInitializeOp_BoundNormalCylinder
//##############################################################################
/// Initializes normals of boundary particles.
class JDsInitializeOp_BoundNormalCylinder : public JDsInitializeOp
{
private:
  std::string MkBound;
  tfloat3 Center1;
  tfloat3 Center2;
  float Radius;
  bool Inside;      ///<Boundary particles inside the cylinder.
  bool Corner;
  float MaxDisteH;  ///<Maximum distance to boundary limit. It uses H*distanceh (default=2).
public:
  JDsInitializeOp_BoundNormalCylinder(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JDsInitializeOp(IT_BoundNormalCylinder,"BoundNormalCylinder",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset();
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner);
  void GetConfig(std::vector<std::string> &lines)const;
};  


//##############################################################################
//# JDsInitialize
//##############################################################################
/// \brief Manages the info of particles from the input XML file.
class JDsInitialize  : protected JObject
{
private:
  const bool BoundNormals;
  const JDsInitializeOp::StInitCt InitCt;  ///<Constant values needed for initialization tasks.
  std::vector<JDsInitializeOp*> Opes;

  void LoadFileXml(const std::string &file,const std::string &path);
  void LoadXml(const JXml *sxml,const std::string &place);
  void ReadXml(const JXml *sxml,TiXmlElement* lis);

public:
  JDsInitialize(const JXml *sxml,const std::string &place
    ,const std::string &dirdatafile,float kernelh,float dp,unsigned nbound
    ,bool boundnormals);
  ~JDsInitialize();
  void Reset();
  unsigned Count()const{ return(unsigned(Opes.size())); }

  void Run(unsigned np,unsigned npb,const tdouble3 *pos
    ,const unsigned *idp,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal,bool *BoundCorner);
  void GetConfig(std::vector<std::string> &lines)const;

};

#endif


