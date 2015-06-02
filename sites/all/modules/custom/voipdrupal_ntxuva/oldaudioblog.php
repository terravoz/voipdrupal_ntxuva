<?php
/**
 * Created by PhpStorm.
 * User: tamerzoubi
 * Date: 5/23/15
 * Time: 10:49 PM
 */

/**
 * @file
 * Audio Blog VoIP Scripts and helper functions
 */

/**
 * Implementation of hook_voipscript_get_script_names()
 */
function audioblog_voipscript_get_script_names() {
  $script_names[] = 'audioblog_main_menu_script';
  $script_names[] = 'audioblog_sms_handler_script';
  return $script_names;
}

/**
 * Implementation of hook_voipscript_load_script()
 */
function audioblog_voipscript_load_script($script_name, $options=NULL) {

  $script = NULL;
  switch ($script_name) {
    default:
      break;
    case 'audioblog_main_menu_script':
      //Audio blog script for Voice channel
      $script = new VoipScript('audioblog_main_menu_script');
      $script->addSetVoice('woman');
      voipvoice_set_current_voice('woman');

      $script->addSay(v('Bem-vindo ao MOPA.')); // welcome to MOPA
      $script->addLabel('main_menu');
      $p_options_menu = v('Para criar uma nova reclama√ß√£o, marque 1. Para saber o estado da sua reclama√ß√£o, marque 2.'); // to create a new request, dial 1. To check the status of your request, dial 2.

      $input_options = array(
        '1' => 'create_request',
        '2' => 'check_request'
      );
      $p_invalid_msg = v('Deve seleccionar uma opção válida.'); // Invalid option
      $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');

      $script->addLabel('play_audio_blogs');
      $script->addGosub('audioblog_play_blogs_script', $options);
      $script->addGoto('main_menu');

      $script->addLabel('record_audio_blog');
      $script->addGosub('audioblog_record_blog_script', $options);
      $script->addGoto('main_menu');

      $script->addLabel('create_request');
      $script->addGosub('create_request_script', $options);
      $script->addGoto('main_menu');

      $script->addLabel('check_request');
      $script->addGosub('check_request_script', $options);
      $script->addGoto('main_menu');

      $script->addLabel('end');
      $script->addSay(v('Thank you for calling. Bye bye.'));
      $script->addHangup();
      break;

    case 'audioblog_play_blogs_script':
      //Helper script that provides play menu for Audio blogs.
      $script = new VoipScript('audioblog_play_blogs_script');
      $script->addSetVoice('woman');
      voipvoice_set_current_voice('woman');

      $blogs = _audioblog_get_blogs();
      if (!$blogs) {
        $script->addSay(v('There are no audio blogs at this time. '));
      }
      else {
        $script->addSay(v('During playback, press any key to go to the audio blog menu. '));
        foreach ($blogs as $index => $blog) {
          $body = $blog['body'];
          $changed = $blog['changed'];

          $args = array('@date' => VoipVoice::getDate($blog['changed']), '@time' => date("g:i a", $blog['changed']));
          $info = v('This audio blog was created on @date at @time', $args);
          $current = "audio_blog_$index";
          $current_info = "info_$index";
          $next = 'audio_blog_' . ($index+1);

          $script->addLabel($current);
          $script->addGetInput($body, 1, '#', 3);

          $script->addLabel('playback_menu');
          $p_options_menu = v('To replay the current audio blog, press 1. To listen to the additional information about this audio blog press 2. To go to the next audio blog, press 3. To go back to the main menu, press the pound key. ');
          $input_options = array(
            '1' => $current,
            '2' => $current_info,
            '3' => $next,
            '#' => 'go_back',
          );
          $p_invalid_msg = v('Invalid option selected');
          $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);
          $script->addGoto('%ivr_option_selected');

          $script->addLabel($current_info);
          $script->addSay($info);
          $script->addGoto('playback_menu');
        }
      }

      $no_more = count($blogs);
      $script->addLabel("audio_blog_$no_more");
      $script->addSay(v('No more audio blogs to be played. '));
      $script->addGoto('go_back');

      $script->addLabel('invalid_option');
      $script->addGoto('go_back');

      $script->addLabel('go_back');
      $script->addReturn();
      break;

    case 'audioblog_record_blog_script':
      //Helper script that prompts user to record a message
      $script = _audioblog_get_record_script();
      break;

    case 'create_request_script':
      $script = new VoipScript('audioblog_create_request_script');
      $script->addSetVoice('woman');
      voipvoice_set_current_voice('woman');

      $script->addLabel('select_category');
      $prompt = v("Por favor, seleccione a categoria do problema. Para "caminhão não recolheu o contentor", marque 1. Para Contentor a Arder, marque 2. Para Lixo fora do contentor, marque 3. Para avisar que Tchova não passou, marque 4. Para identificar Lixeira informal, marque 5. Para voltar ao menu anterior, marque estrela"); // Please choose the category of the request. If the truck did not collect the container, dial 1. If the container is burning, dial 2. If there is garbage outside the container, dial 3. To inform that the Tchova has not passed, dial 4. To identify an informal garbage container, dial 5. To go back to the previous menu, dial star.
    	$timeout = 5;
    	$end_key = '';
    	$num_digits = 1;
    	$script->addGetInput($prompt, $num_digits, $end_key, $timeout);

    	$script->addGotoIf('go_back', "^%input_digits == '*'");

    	$script->addSet('service_code', '%input_digits');


        $script->addLabel('select_container');
    	$prompt = v("Por favor, introduza o número de contentor. Para voltar ao início, marque estrela"); // Please, dial the number of the container. To go back to the beginning, dial star.
    	$timeout = 5;
    	$end_key = '';
    	$num_digits = 2;
    	$script->addGetInput($prompt, $num_digits, $end_key, $timeout);
        $script->addGotoIf('go_back', "^%input_digits == '*'");
        $script->addSet('location', '%input_digits');

                $script->addSet('voipscript_hangup_callback', '_audioblog_record_on_hang_up');
                $prompt = v("Por favor grave a sua mensagem a seguir ao sinal. Para terminar pressione estrela."); // Please, record your message after the beep. To end, press star.
            	$timeout = 5;
            	$end_key = '*';
            	$max_length = 3600; //1 hour
                $script->addRecord($prompt, $timeout, $end_key, $max_length);

            	// reset the hang up handler
                $script->addSet('voipscript_hangup_callback', '');

            	$args = array('@message' => '%recording_public_url');
                $script->addSay(v('A sua mensagem é a seguinte: @message', $args)); // This is what you recorded:

                $script->addLabel('accept menu');
            	$prompt = v("Para aceitar, pressione 1.  Para gravar novamente, pressione 2."); // To accept, dial 1. To record once again, press 2.
                $timeout = 5;
                $end_key = '';
                $num_digits = 1;
                $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

            	$script->addGotoIf('accept recording', "^%input_digits == '1'");

            	$script->addGotoIf('start', "^%input_digits == '2'");

            	$script->addSay(v('Invalid input received. '));
                $script->addSay(v('Please try again.'));
            	$script->addGoto('accept menu');

                $script->addLabel('accept recording');
        	    $script->addSay(v('Vamos agora processar o seu pedido.')); // About to process your request
                $script->addSet('callback_result',
                  '^_audioblog_record_callback(%recording_public_url, %source, %cid, %recording_duration)');

                $script->addSay('%callback_result');

    	$script->addSet('phone', '962404042');


        $script->addSet('reply', '^_audioblog_create_request (%phone, %location, %service_code, %recording_public_url)');
    	$script->addSay('%reply');
    	$script->addHangup();
	break;

    case 'check_request_script':
      $script = new VoipScript('audioblog_check_request_script');
      $script->addSetVoice('woman');
      voipvoice_set_current_voice('woman');

      $script->addLabel('insert_request_number');
      $prompt = v("Por favor, introduza o número de pedido que pretende consultar."); // Please, type the number of the request that you are looking for
      $timeout = 5;
      $end_key = '';
      $num_digits = 7;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addSet('request', '%input_digits');
      $script->addSet('request_status', '^_audioblog_get_request_status(%request)');

      $script->addGotoIf('request_not_found', "^%request_status == '0'");
      $script->addGotoIf('request_open', "^%request_status == 'open'");
      $script->addGotoIf('request_closed', "^%request_status == 'closed'");

      $script->addLabel('request_open');
      $script->addSay('O seu pedido encontra-se aberto. Será notificado assim que a situação for resolvida. Obrigado pelo seu telefonema.'); // Your request is still open. You will be notified as soon as your situation gets resolved. Thank you for calling.
      $script->addGoto('hangup');

      $script->addLabel('request_closed');
      $script->addSay('O seu pedido encontra-se resolvido. Obrigado pelo seu telefonema.'); // Your request has been solved. Thank you for your call.
      $script->addGoto('hangup');

      $script->addLabel('request_not_found');

      $p_options_menu = v('O seu pedido não foi encontrado. Lamentamos o incômodo. Se quiser tentar novamente, marque 2. Para voltar ao iníciocio, marque estrela. Caso contrário, por favor desligue.'); // We are sorry, but your request has not been found.  To try again, dial 2. To go back to the beginning, dial star. Once you are done, please hangup.
      $input_options = array(
        '2' => 'insert_request_number',
        '*' => 'insert_request_number',
      );
      $p_invalid_msg = v('A opção que seleccionou é inválida, por favor tente novamente.'); // Invalid option, please try again
      $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');


      $script->addLabel('hangup');
      $script->addHangup();

      break;

    case 'audioblog_sms_handler_script':
      //Audio blog script for Text channel
      $script = new VoipScript('audioblog_sms_handler_script');
      // log the sms
      $log_msg = t("SMS from %caller_number on @date (id: %call_id)",
        array('@date' => format_date(time())));
      $script->addLog($log_msg, 'MDT');

      // below was the only way to avoid problems with special characters in the text message
      $options['text'] = '%inbound_text_contents';
      $script->addGosub('audioblog_sms_process_request_script', $options);
      break;

    case 'audioblog_sms_process_request_script':
      //Helper script to process SMS request
      $script = new VoipScript('audioblog_sms_process_request_script');
      $result = _audioblog_sms_process_request($options['text']);
      // send response in chunks of 160 characteres
      if (strlen($result) <= 160) {
        $text_array[] = $result;
      }
      else {
        $tmp = wordwrap($result, 160, '\n');
        $text_array = explode('\n', $tmp);
      }
      foreach ($text_array as $text) {
        $script->addSendText($text);
        $script->addLog("sending $text");
      }
      $script->addHangup();
      break;
  }

  return $script;
}

function _audioblog_create_request ($phone, $location, $service_code, $recording_public_url) {
  global $base_url;
  $ntxuva_default_endpoint = "demo.ntxuva.org/georeport/v2/";
  $endpoint = variable_get('nxtuva_open311_endpoint', $ntxuva_default_endpoint);

  $url = "http://{$endpoint}requests.json";

  $location_result = taxonomy_get_term_by_name('0'.$location,'points');

  watchdog("ntxuva_sms", "Location 1: " . print_r($location,true)); // save request for debugging
  watchdog("ntxuva_sms", "Location 2: " . print_r($location_result,true)); // save request for debugging


  // Validate location
  if(empty($location_result)) {
    $reply = "Lamentavelmente, nao conseguimos introduzir o seu pedido. O seu ponto de recolha nao foi reconhecido.";
    return ($reply);
  }

  else {

    $tid = key($location_result);
    $location_result = $location_result[$tid];

    $loc = entity_metadata_wrapper('taxonomy_term', $location_result);

    watchdog("ntxuva_sms", "Location 3: " . print_r($loc,true)); // save request for debugging


    $lat = $loc->field_lat->value();
    $long = $loc->field_long->value();
    if(!empty($loc->field_address_string)) $address_string = $loc->field_address_string->value();
  }

  // Prepare JSON

  $request = array(
    'phone'  	=> $phone,
    'long'    => $long,
    'lat'   	=> $lat,
    'description' => "Gerado por telefone",
    'service_code'  	=> '0'.$service_code,
    'address_string'  => $address_string,
    'media_url'	=> $recording_public_url,
  );

  $options = array(
    'http' => array(
      'method'  => 'POST',
      'content' => json_encode( $request ),
      'header'=>  "Content-Type: application/json\r\n" .
        "Accept: application/json\r\n"
    )
  );

  watchdog("ntxuva_sms", "Preparing JSON: " . print_r($request,true)); // save request for debugging

  $context  = stream_context_create( $options );

  // Send JSON POST to Submit issue

  $result = file_get_contents($url, false, $context);
  $res = json_decode($result, true);

  // Define reply message
  // If error, send verbose message to user.

  if (isset($res[0]['service_request_id'])) {
    $reply = "Obrigado pela sua mensagem! O seu pedido foi guardado com o codigo: " . $res[0]['service_request_id'];
  }

  // If not, apologize
  else {
    watchdog("ntxuva_sms", "Open311: could not create request"); // save request for debugging
    $reply = ("Lamentavelmente, nao conseguimos introduzir o seu pedido, por favor tente mais tarde.");
  }

  return ($reply);

}

function _audioblog_get_request_status ($request) {
  global $base_url;
  $ntxuva_default_endpoint = "demo.ntxuva.org/georeport/v2/";
  $endpoint = variable_get('nxtuva_open311_endpoint', $ntxuva_default_endpoint);

  $part = str_split($request, 4);


  $url = "http://{$endpoint}requests/{$part[0]}-{$part[1]}.json";


  // Request status from Open311 Endpoint
  $json = file_get_contents($url);
  $res = json_decode($json, true);

  // If successful, send status

  if (isset($res[0]['status'])) {
    return $res[0]['status'];
    watchdog("ntxuva_sms", "AudioOpen311: Got response from Open311: " .print_r($res,true)); // save request for debugging
  }
  else {
    return 0;
  }
}

/*
* Returns array of Audioblog nodes
*/
function _audioblog_get_blogs() {
  $blogs = array();

  //$sql = "SELECT n.nid AS nid FROM {node} n WHERE (n.type = 'audioblog') AND (n.status <> 0) ORDER BY n.changed DESC";
  $result = db_select('node', 'n')
    ->fields('n', array('nid'))
    ->condition('type', 'audioblog')
    ->condition('status', 0, '<>')
    ->orderBy('changed', 'DESC')
    ->execute();

  foreach ($result as $o) {
    $n = node_load($o->nid);
    $blog = array();
    $blog['nid'] = $n->nid;
    $blog['changed'] = $n->changed;

    $field_audio = $n->field_audioblog_recording;
    if ($field_audio['und'][0]) {
      $audio_url = file_create_url($field_audio['und'][0]['uri']);
      $blog['body'] = $audio_url;
    }

    if (!$blog['body']) {
      //If no audio then load text version
      $blog['body'] = $n->body['und'][0]['value'];
    }

    $blogs[] = $blog;
  }

  return $blogs;
}

function _audioblog_get_record_script() {
  $script = new VoipScript('audioblog_record_blog_script');

  $script->addSetVoice('woman');
  voipvoice_set_current_voice('woman');

  $script->addLabel('start');
  // prepare the call to finish processing the recording even in case of hang up
  $script->addSet('voipscript_hangup_callback', '_audioblog_record_on_hang_up');
  $prompt = v("Please record your audio blog after the beep.  When done, either hang up or press the pound key for additional options.");
  $timeout = 5;
  $end_key = '#';
  $max_length = 3600; //1 hour
  $script->addRecord($prompt, $timeout, $end_key, $max_length);

  // reset the hang up handler
  $script->addSet('voipscript_hangup_callback', '');

  $args = array('@message' => '%recording_public_url');
  $script->addSay(v('You said @message', $args));

  $script->addLabel('accept menu');
  $prompt = v("To accept this recording, press 1.  To record it once again, press 2. To hangup, press the pound key.");
  $timeout = 5;
  $end_key = '';
  $num_digits = 1;
  $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

  $script->addGotoIf('accept recording', "^%input_digits == '1'");

  $script->addGotoIf('start', "^%input_digits == '2'");

  $script->addGotoIf('end call', "^%input_digits == '#'");

  $script->addSay(v('Invalid input received. '));
  $script->addSay(v('Please try again.'));
  $script->addGoto('accept menu');

  $script->addLabel('accept recording');
  $script->addSay(v('About to start processing your recording.'));
  $script->addSet('callback_result',
    '^_audioblog_record_callback(%recording_public_url, %source, %cid, %recording_duration)');

  $script->addSay('%callback_result');
  $script->addGoto('end call');

  $script->addLabel('no input received');
  $script->addSay(v("No input received. "));
  $script->addSay(v('Please try again.'));
  $script->addGoto('start');

  $script->addLabel('end call');
  $script->addSay(v('Bye bye.'));
  $script->addHangup();

  return $script;
}

/**
 * Function executed on hang up as part of audioblog_record_blog_script script
 */
function _audioblog_record_on_hang_up($original_call) {
  $original_script = $original_call->getScript();
  $cid = $original_call->getCid();
  $source = $original_call->getSource();
  $recording_public_url = $original_script->getVar('recording_public_url');
  $recording_duration = $original_script->getVar('recording_duration');
  $result = _audioblog_record_callback($recording_public_url, $source, $cid, $recording_duration);
}

/**
 * Callback function associated with the script audioblog_record_blog_script.  It is called
 *  whenever that script records a new entry from the user and it creates a new Audioblog node.
 */
function _audioblog_record_callback($recording_public_url, $source, $cid, $duration) {
  $server = Voipserver::getServer($source);
  $call = VoipCall::load($cid);
  $args = array(
    'call' => $call,
    'duration' => $duration,
  );
  $server->audioFileMover($recording_public_url, '_audioblog_create_audioblog', $args);
  $msg = t('');
  return $msg;
}

/**
 * Process incoming SMS messages
 */
function _audioblog_sms_process_request($msg) {
  $help_msg = t("To submit a new Blog, please text ADD followed by space and the actual blog content.");
  $msg = trim($msg);
  if (!$msg) {
    $ret = t("Empty request.") . ' ' . $help_msg;
  }
  else {
    $request = explode(" ", $msg);
    $cmd = strtoupper($request[0]);
    if ($cmd == '?') {
      $ret = $help_msg;
    }
    elseif ($cmd == 'ADD') {
      unset($request[0]);
      $text = implode(" ", $request);
      $node = _audioblog_create_textblog($text);
      $options = array('@link' => url('node/' . $node->nid, array('absolute' => TRUE)));
      $ret = t("Your Blog has been added to location @link . Thanks!", $options);
    }
    else {
      $ret = t("Invalid request.") . ' ' . $help_msg;
    }
  }
  return $ret;
}

/**
 * Create blog node from text message.
 *
 * @param string $text.
 *   Content of text message.
 *
 * @return
 *   Node object.
 */
function _audioblog_create_textblog($text) {
  $node->type = 'audioblog';
  $node->promote = TRUE;

  $options = array('@datetime' => date('l, F j, Y \a\t g:i a', time()));
  $node->title = t('Audioblog entry created on @datetime', $options);

  $node->body['und'][0]['value'] = $text;

  // create the node
  node_save($node);
  return $node;
}

/**
 * Create blog node from audio message.
 *
 * @param object file.
 *   Drupal file object.
 *
 * @return
 *   Node object.
 */
function _audioblog_create_audioblog($file, $recording_public_url, $args) {
  $node->type = 'audioblog';
  $node->promote = TRUE;
  $node->language = 'und';

  $options = array('@datetime' => date('l, F j, Y \a\t g:i a', time()));
  $node->title = t('Audioblog entry created on @datetime', $options);

  $file->display = 1;
  $node->field_audioblog_recording['und'][0] = (array)$file;

  // create the node
  node_save($node);
  return $node;
}

