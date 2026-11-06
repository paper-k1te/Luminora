Shader "Custom/VoxelBlock"
{
    Properties
    {
        _MainTex ("Voxel Texture Atlas", 2D) = "white" {}
        _EmissionMap ("Emission Map", 2D) = "black" {}
        _EmissionColor ("Emission Color", Color) = (1, 1, 1, 1)
        _Cutoff ("Alpha Cutoff", Range(0, 1)) = 0.35
        _Color ("Main Tint", Color) = (1, 1, 1, 1)
    }

    SubShader
    {
        Tags 
        { 
            "Queue" = "AlphaTest" 
            "IgnoreProjector" = "True" 
            "RenderType" = "TransparentCutout" 
        }
        LOD 150
        Cull Back
        ZWrite On

        Pass
        {
            Name "ForwardBase"
            Tags { "LightMode" = "ForwardBase" }

            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile_fog
            #include "UnityCG.cginc"
            #include "Lighting.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float3 normal : NORMAL;
                float4 color : COLOR;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float4 pos : SV_POSITION;
                fixed4 color : COLOR;
                float2 uv : TEXCOORD0;
                UNITY_FOG_COORDS(1)
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;
            sampler2D _EmissionMap;
            float4 _EmissionMap_ST;
            fixed4 _EmissionColor;
            fixed _Cutoff;
            fixed4 _Color;

            v2f vert(appdata v)
            {
                v2f o;
                o.pos = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);

                // Simple directional lighting + vertex ambient occlusion
                half3 worldNormal = UnityObjectToWorldNormal(v.normal);
                half nl = max(0.0, dot(worldNormal, _WorldSpaceLightPos0.xyz));
                fixed3 diffuseLight = nl * _LightColor0.rgb + ShadeSH9(half4(worldNormal, 1.0));

                // Modulate by baked bevel/ambient vertex color
                o.color.rgb = v.color.rgb * diffuseLight * _Color.rgb;
                o.color.a = v.color.a * _Color.a;

                UNITY_TRANSFER_FOG(o, o.pos);
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                fixed4 tex = tex2D(_MainTex, i.uv);
                clip(tex.a - _Cutoff);

                fixed4 col;
                col.rgb = tex.rgb * i.color.rgb;
                col.a = tex.a * i.color.a;

                // Additive emissive glow (magma, glowshrooms, torches, ores)
                fixed4 emit = tex2D(_EmissionMap, i.uv) * _EmissionColor;
                col.rgb += emit.rgb;

                UNITY_APPLY_FOG(i.fogCoord, col);
                return col;
            }
            ENDCG
        }
    }

    Fallback "Unlit/Transparent Cutout"
}
