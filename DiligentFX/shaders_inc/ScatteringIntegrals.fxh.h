"\n"
"float2 GetNetParticleDensity(in float fHeightAboveSurface,\n"
"                             in float fCosZenithAngle,\n"
"                             in float fAtmTopHeight)\n"
"{\n"
"    float fRelativeHeightAboveSurface = fHeightAboveSurface / fAtmTopHeight;\n"
"    return g_tex2DOccludedNetDensityToAtmTop.SampleLevel(g_tex2DOccludedNetDensityToAtmTop_sampler, float2(fRelativeHeightAboveSurface, fCosZenithAngle*0.5+0.5), 0);\n"
"}\n"
"\n"
"float2 GetNetParticleDensity(in float3 f3Pos,\n"
"                             in float3 f3EarthCentre,\n"
"                             in float  fEarthRadius,\n"
"							 in float  fAtmTopHeight,\n"
"                             in float3 f3RayDir)\n"
"{\n"
"    float3 f3EarthCentreToPointDir = f3Pos - f3EarthCentre;\n"
"    float fDistToEarthCentre = length(f3EarthCentreToPointDir);\n"
"    f3EarthCentreToPointDir /= fDistToEarthCentre;\n"
"    float fHeightAboveSurface = fDistToEarthCentre - fEarthRadius;\n"
"    float fCosZenithAngle = dot( f3EarthCentreToPointDir, f3RayDir );\n"
"    return GetNetParticleDensity(fHeightAboveSurface, fCosZenithAngle, fAtmTopHeight);\n"
"}\n"
"\n"
"void ApplyPhaseFunctions(inout float3 f3RayleighInscattering,\n"
"                         inout float3 f3MieInscattering,\n"
"                         in float cosTheta)\n"
"{\n"
"    f3RayleighInscattering *= g_MediaParams.f4AngularRayleighSctrCoeff.rgb * (1.0 + cosTheta*cosTheta);\n"
"    \n"
"    // Apply Cornette-Shanks phase function (see Nishita et al. 93):\n"
"    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)\n"
"    // f4CS_g = ( 3*(1-g^2) / (2*(2+g^2)), 1+g^2, -2g, 1 )\n"
"    float fDenom = rsqrt( dot(g_MediaParams.f4CS_g.yz, float2(1.0, cosTheta)) ); // 1 / (1 + g^2 - 2g*cos(theta))^(1/2)\n"
"    float fCornettePhaseFunc = g_MediaParams.f4CS_g.x * (fDenom*fDenom*fDenom) * (1.0 + cosTheta*cosTheta);\n"
"    f3MieInscattering *= g_MediaParams.f4AngularMieSctrCoeff.rgb * fCornettePhaseFunc;\n"
"}\n"
"\n"
"// This function computes atmospheric properties in the given point\n"
"void GetAtmosphereProperties(in float3  f3Pos,\n"
"                             in float3  f3EarthCentre,\n"
"                             in float   fEarthRadius,\n"
"                             in float   fAtmTopHeight,\n"
"							 in float4  f4ParticleScaleHeight,\n"
"                             in float3  f3DirOnLight,\n"
"                             out float2 f2ParticleDensity,\n"
"                             out float2 f2NetParticleDensityToAtmTop)\n"
"{\n"
"    // Calculate the point height above the SPHERICAL Earth surface:\n"
"    float3 f3EarthCentreToPointDir = f3Pos - f3EarthCentre;\n"
"    float fDistToEarthCentre = length(f3EarthCentreToPointDir);\n"
"    f3EarthCentreToPointDir /= fDistToEarthCentre;\n"
"    float fHeightAboveSurface = fDistToEarthCentre - fEarthRadius;\n"
"\n"
"    f2ParticleDensity = exp( -fHeightAboveSurface * f4ParticleScaleHeight.zw );\n"
"\n"
"    // Get net particle density from the integration point to the top of the atmosphere:\n"
"    float fCosSunZenithAngleForCurrPoint = dot( f3EarthCentreToPointDir, f3DirOnLight );\n"
"    f2NetParticleDensityToAtmTop = GetNetParticleDensity(fHeightAboveSurface, fCosSunZenithAngleForCurrPoint, fAtmTopHeight);\n"
"}\n"
"\n"
"// This function computes differential inscattering for the given particle densities \n"
"// (without applying phase functions)\n"
"void ComputePointDiffInsctr(in float2 f2ParticleDensityInCurrPoint,\n"
"                            in float2 f2NetParticleDensityFromCam,\n"
"                            in float2 f2NetParticleDensityToAtmTop,\n"
"                            out float3 f3DRlghInsctr,\n"
"                            out float3 f3DMieInsctr)\n"
"{\n"
"    // Compute total particle density from the top of the atmosphere through the integraion point to camera\n"
"    float2 f2TotalParticleDensity = f2NetParticleDensityFromCam + f2NetParticleDensityToAtmTop;\n"
"        \n"
"    // Get optical depth\n"
"    float3 f3TotalRlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2TotalParticleDensity.x;\n"
"    float3 f3TotalMieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2TotalParticleDensity.y;\n"
"        \n"
"    // And total extinction for the current integration point:\n"
"    float3 f3TotalExtinction = exp( -(f3TotalRlghOpticalDepth + f3TotalMieOpticalDepth) );\n"
"\n"
"    f3DRlghInsctr = f2ParticleDensityInCurrPoint.x * f3TotalExtinction;\n"
"    f3DMieInsctr  = f2ParticleDensityInCurrPoint.y * f3TotalExtinction; \n"
"}\n"
"\n"
"void ComputeInsctrIntegral(in float3    f3RayStart,\n"
"                           in float3    f3RayEnd,\n"
"                           in float3    f3EarthCentre,\n"
"                           in float     fEarthRadius,\n"
"                           in float     fAtmTopHeight,\n"
"						   in float4    f4ParticleScaleHeight,\n"
"                           in float3    f3DirOnLight,\n"
"                           in uint      uiNumSteps,\n"
"                           inout float2 f2NetParticleDensityFromCam,\n"
"                           inout float3 f3RayleighInscattering,\n"
"                           inout float3 f3MieInscattering)\n"
"{\n"
"    float3 f3Step = (f3RayEnd - f3RayStart) / float(uiNumSteps);\n"
"    float fStepLen = length(f3Step);\n"
"\n"
"#if TRAPEZOIDAL_INTEGRATION\n"
"    // For trapezoidal integration we need to compute some variables for the starting point of the ray\n"
"    float2 f2PrevParticleDensity = float2(0.0, 0.0);\n"
"    float2 f2NetParticleDensityToAtmTop = float2(0.0, 0.0);\n"
"    GetAtmosphereProperties(f3RayStart, f3EarthCentre, fEarthRadius, fAtmTopHeight, f4ParticleScaleHeight, f3DirOnLight, f2PrevParticleDensity, f2NetParticleDensityToAtmTop);\n"
"\n"
"    float3 f3PrevDiffRInsctr = float3(0.0, 0.0, 0.0), f3PrevDiffMInsctr = float3(0.0, 0.0, 0.0);\n"
"    ComputePointDiffInsctr(f2PrevParticleDensity, f2NetParticleDensityFromCam, f2NetParticleDensityToAtmTop, f3PrevDiffRInsctr, f3PrevDiffMInsctr);\n"
"#endif\n"
"\n"
"\n"
"    for (uint uiStepNum = 0u; uiStepNum < uiNumSteps; ++uiStepNum)\n"
"    {\n"
"#if TRAPEZOIDAL_INTEGRATION\n"
"        // With trapezoidal integration, we will evaluate the function at the end of each section and \n"
"        // compute area of a trapezoid\n"
"        float3 f3CurrPos = f3RayStart + f3Step * (float(uiStepNum) + 1.0);\n"
"#else\n"
"        // With stair-step integration, we will evaluate the function at the middle of each section and \n"
"        // compute area of a rectangle\n"
"        float3 f3CurrPos = f3RayStart + f3Step * (float(uiStepNum) + 0.5);\n"
"#endif\n"
"        \n"
"        float2 f2ParticleDensity, f2NetParticleDensityToAtmTop;\n"
"        GetAtmosphereProperties(f3CurrPos, f3EarthCentre, fEarthRadius, fAtmTopHeight, f4ParticleScaleHeight, f3DirOnLight, f2ParticleDensity, f2NetParticleDensityToAtmTop);\n"
"\n"
"        // Accumulate net particle density from the camera to the integration point:\n"
"#if TRAPEZOIDAL_INTEGRATION\n"
"        f2NetParticleDensityFromCam += (f2PrevParticleDensity + f2ParticleDensity) * (fStepLen / 2.0);\n"
"        f2PrevParticleDensity = f2ParticleDensity;\n"
"#else\n"
"        f2NetParticleDensityFromCam += f2ParticleDensity * fStepLen;\n"
"#endif\n"
"\n"
"        float3 f3DRlghInsctr, f3DMieInsctr;\n"
"        ComputePointDiffInsctr(f2ParticleDensity, f2NetParticleDensityFromCam, f2NetParticleDensityToAtmTop, f3DRlghInsctr, f3DMieInsctr);\n"
"\n"
"#if TRAPEZOIDAL_INTEGRATION\n"
"        f3RayleighInscattering += (f3DRlghInsctr + f3PrevDiffRInsctr) * (fStepLen / 2.0);\n"
"        f3MieInscattering      += (f3DMieInsctr  + f3PrevDiffMInsctr) * (fStepLen / 2.0);\n"
"\n"
"        f3PrevDiffRInsctr = f3DRlghInsctr;\n"
"        f3PrevDiffMInsctr = f3DMieInsctr;\n"
"#else\n"
"        f3RayleighInscattering += f3DRlghInsctr * fStepLen;\n"
"        f3MieInscattering      += f3DMieInsctr * fStepLen;\n"
"#endif\n"
"    }\n"
"}\n"
"\n"
"\n"
"void IntegrateUnshadowedInscattering(in float3   f3RayStart, \n"
"                                     in float3   f3RayEnd,\n"
"                                     in float3   f3ViewDir,\n"
"                                     in float3   f3EarthCentre,\n"
"                                     in float    fEarthRadius,\n"
"                                     in float    fAtmTopHeight,\n"
"							         in float4   f4ParticleScaleHeight,\n"
"                                     in float3   f3DirOnLight,\n"
"                                     in uint     uiNumSteps,\n"
"                                     out float3  f3Inscattering,\n"
"                                     out float3  f3Extinction)\n"
"{\n"
"    float2 f2NetParticleDensityFromCam = float2(0.0, 0.0);\n"
"    float3 f3RayleighInscattering = float3(0.0, 0.0, 0.0);\n"
"    float3 f3MieInscattering = float3(0.0, 0.0, 0.0);\n"
"    ComputeInsctrIntegral( f3RayStart,\n"
"                           f3RayEnd,\n"
"                           f3EarthCentre,\n"
"                           fEarthRadius,\n"
"                           fAtmTopHeight,\n"
"                           f4ParticleScaleHeight,\n"
"                           f3DirOnLight,\n"
"                           uiNumSteps,\n"
"                           f2NetParticleDensityFromCam,\n"
"                           f3RayleighInscattering,\n"
"                           f3MieInscattering);\n"
"\n"
"    float3 f3TotalRlghOpticalDepth = g_MediaParams.f4RayleighExtinctionCoeff.rgb * f2NetParticleDensityFromCam.x;\n"
"    float3 f3TotalMieOpticalDepth  = g_MediaParams.f4MieExtinctionCoeff.rgb      * f2NetParticleDensityFromCam.y;\n"
"    f3Extinction = exp( -(f3TotalRlghOpticalDepth + f3TotalMieOpticalDepth) );\n"
"\n"
"    // Apply phase function\n"
"    // Note that cosTheta = dot(DirOnCamera, LightDir) = dot(ViewDir, DirOnLight) because\n"
"    // DirOnCamera = -ViewDir and LightDir = -DirOnLight\n"
"    float cosTheta = dot(f3ViewDir, f3DirOnLight);\n"
"    ApplyPhaseFunctions(f3RayleighInscattering, f3MieInscattering, cosTheta);\n"
"\n"
"    f3Inscattering = f3RayleighInscattering + f3MieInscattering;\n"
"}\n"