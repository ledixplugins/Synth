; Instala o plugin VST3 junto com o app e remove na desinstalação.
; O produto se chamava AURA Synth Bass: o plugin com o nome antigo é removido para não aparecer duas vezes na DAW.
!macro customInstall
  SetRegView 64
  CreateDirectory "$COMMONFILES64\VST3"
  RMDir /r "$COMMONFILES64\VST3\AURA Synth Bass.vst3"
  RMDir /r "$COMMONFILES64\VST3\AURA Synth.vst3"
  CopyFiles /SILENT "$INSTDIR\resources\plugins\AURA Synth.vst3" "$COMMONFILES64\VST3"
!macroend
!macro customUnInstall
  SetRegView 64
  RMDir /r "$COMMONFILES64\VST3\AURA Synth.vst3"
  RMDir /r "$COMMONFILES64\VST3\AURA Synth Bass.vst3"
!macroend
