<?php 
/**
 * @file
 * Implementation of VoIP Voice class
 */

class VoipVoiceLocal extends VoipVoice {

 /**
 * Constructor
 */
 function __construct($voice_id, $gender, $language) {
    $is_local = TRUE;
    parent::__construct($voice_id, $gender, $language, $is_local);
 }

 /*
  * Public methods
  */

  /**
   * Extends VoipVoice::getPrompt() to return the audio files associated
   * with the local voices
   */
  function getPrompt($text, $args = NULL, $as = NULL) {
    //Use voipvoice function to get locally recorder voice
    $url_or_text = voipvoice_get_prompt($text, $args, $this->getVoiceId());
    //get voice id in case local voice is not recorded.
    /*if(!$as) {
      $voice = $this->getBestMatchedVoice();
      $voice_id = $voice->getVoiceId();
    }
    else {*/
      $voice_id = VoipVoice::getVoice($as)->getVoiceId();
    //}
    return new VoipPrompt($url_or_text, $voice_id, $as);
  }

  function getBestMatchedVoice($voice_id = NULL) {
    //get best match for this local voice, based on language.
    $lang = $this->getLanguage();
    $voipvoices = VoipVoice::getVoices($lang, NULL, FALSE, NULL);
    unset($voipvoices['log']);
    unset($voipvoices['woman']);
    unset($voipvoices['man']);

    $voice = reset($voipvoices);
    return $voice;
  }
}
