<?php
// $Id$
/**
 * @file
 * Ntxuva VoIP Scripts and helper functions
 */

/**
 * Implementation of hook_voipscript_get_script_names()
 */
function voipdrupal_ntxuva_voipscript_get_script_names() {
  $script_names[] = 'voipdrupal_ntxuva_welcome_script';
  $script_names[] = 'voipdrupal_ntxuva_sms_callmeback_script';

  return $script_names;
}

/**
 * Implementation of hook_voipscript_load_script()
 */
function voipdrupal_ntxuva_voipscript_load_script($script_name, $options = NULL) {
  $script = NULL;
  switch ($script_name) {
    default:
      break;
    case 'voipdrupal_ntxuva_sms_callmeback_script':
      $script = new VoipScript('voipdrupal_ntxuva_sms_callmeback_script');
      $script->addDial();
      $script->addHangup();
      break;
    case 'voipdrupal_ntxuva_welcome_script':
      $script = new VoipScript('voipdrupal_ntxuva_welcome_script');
      $script->addSetVoice('woman');
      $options['voice'] = 'voipdrupal_ntxuva';
      $options['direction'] = '%direction';
      $options['dest_number'] = '%dest_number';
      $options['caller_number'] = '%caller_number';
      voipvoice_set_current_voice('ntxuva-portuguese');
      $script->addSet('max_session_duration', 0);

      // log the call
      $log_msg = t("Call from %caller_number on @date (id: %call_id)",
        array('@date' => format_date(time(), 'MDT')));
      $script->addLog($log_msg, 'voipdrupal_ntxuva');

      $script->addLabel('welcome');
      $script->addSay(v('Welcome to the MOPA system. '));

      $script->addLabel('language_selection');
      $p_main_menu[] = v("For Portuguese, dial 1. ");
      $p_main_menu[] = v("For Changana, dial 2. ");
      $p_main_menu[] = v("For English, dial 3. ");
      $input_options = array(
        '1' => 'portuguese_version',
        '2' => 'changana_version',
        '3' => 'english_version',
        'i' => 'hang_up',
        't' => 'hang_up'
      );
      $p_invalid_msg = v('Invalid option selected. Please try again. ');
      $script->addRunIvrMenu($p_main_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');

      /*Language selection*/
      $script->addLabel('portuguese_version');
      $options['voice'] = 'ntxuva-portuguese';
      $script->addGosub('voipdrupal_ntxuva_main_menu_script', $options);
      $script->addGoto('hang_up');

      $script->addLabel('changana_version');
      $options['voice'] = 'ntxuva-changana';
      $script->addGosub('voipdrupal_ntxuva_main_menu_script', $options);
      $script->addGoto('hang_up');

      $script->addLabel('english_version');
      $options['voice'] = 'ntxuva-english';
      $script->addGosub('voipdrupal_ntxuva_main_menu_script', $options);
      $script->addGoto('hang_up');

      $script->addLabel('hang_up');
      voipvoice_set_current_voice($options['voice']);
      $script->addSay(v('Thank you for calling the MOPA system.'));
      $script->addHangup();
      break;
    case 'voipdrupal_ntxuva_main_menu_script':
      $script = new VoipScript('voipdrupal_ntxuva_main_menu_script');
      $voice = $options['voice'];
      voipvoice_set_current_voice($voice);

      $script->addLabel('main_menu');
      $p_main_menu[] = v("Main menu. To submit a report, press 1.
      To check the status of a report you submitted before, press 2. To learn more about MOPA, dial the star key.");
      $input_options = array(
        '1' => 'submit_report',
        '2' => 'status_report',
        '*' => 'about_mopa',
        '#' => 'invalid_option',
        'i' => 'invalid_option',
        't' => 'invalid_option'
      );
      $p_invalid_msg = v('Invalid option selected. Please try again. ');
      $script->addRunIvrMenu($p_main_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');

      $script->addLabel('submit_report');
      $script->addGosub('voipdrupal_ntxuva_submit_report_script', $options);
      $script->addReturn();

      $script->addLabel('status_report');
      $script->addGosub('voipdrupal_ntxuva_check_report_script', $options);
      $script->addReturn();

      $script->addLabel('about_mopa');
      $script->addSay(v('About MOPA.'));
      $script->addGoto('main_menu');

      $script->addLabel('invalid_option');
      $script->addSay($p_invalid_msg);
      $script->addGoto('main_menu');

      $script->addReturn();
      break;
    case 'voipdrupal_ntxuva_submit_report_script':
      $script = new VoipScript('voipdrupal_ntxuva_submit_report_script');
      $voice = $options['voice'];
      voipvoice_set_current_voice($voice);

      if($options['direction'] == 'inbound') {
        $phone_number = VoipCall::NormalizeNumber($options['caller_number']);
      }
      else {
        $phone_number = VoipCall::NormalizeNumber($options['dest_number']);
      }
      $script->addSet('phone_number', $phone_number);

      $script->addLabel('submit_welcome');
      $script->addSay(v('You are about to submit a new report to the MOPA system. '));

      $script->addLabel('select_neighborhood');
      $script->addSay(v('Please select the neighbourhood you want to report on. '));

      $prompt = v('To report on Maxaquene A press 1.
      To report on Polana Caniço B press 2.
      To report on Inhagoia B press 3.
      To report on Magoanine C press 4. ');
      $timeout = 5;
      $end_key = '';
      $num_digits = 1;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addGotoIf('set_neighborhood', "^_voipdrupal_ntxuva_validate_neighborhood(%input_digits)");

      $script->addSay(v('Invalid option selected. Please try again. '));
      $script->addGoto('select_neighborhood');

      $script->addLabel('set_neighborhood');
      $script->addSet('neighborhood', '%input_digits');

      $script->addLabel('select_problem_type');
      $voipdrupal_ntxuva_services = variable_get('voipdrupal_ntxuva_services', array());
      $prompt = array();
      $prompt[] = v('Please select the type of the problem being reported. ');
      foreach ($voipdrupal_ntxuva_services as $key => $ntxuva_service) {
        $ivr_key = $key{1};
        $prompt[] = v('For ');
        $prompt[] = v($ntxuva_service);
        $prompt[] = v(' dial ');
        $prompt[] = v($ivr_key);
      }
      $timeout = 5;
      $end_key = '';
      $num_digits = 1;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addGotoIf('set_type', "^_voipdrupal_ntxuva_validate_problem_type(%input_digits)");

      $script->addSay(v('Invalid option selected. Please try again. '));
      $script->addGoto('select_problem_type');

      $script->addLabel('set_type');
      $script->addSet('service_code', '%input_digits');

      $script->addSay(v('You selected: '));
      //@todo: wraping in v doesn't work!
      $script->addSay("^_voipdrupal_ntxuva_problem_type(%input_digits)");

      //Problem type 1,2 and 3 need container id.
      $script->addGotoIf('select_container', "^_voipdrupal_ntxuva_problem_type_needs_container(%input_digits)");
      //Other problem types need quarter
      $script->addGoto('select_quarter');

      $script->addLabel('select_container');
      $options['neighborhood'] = '%neighborhood';
      $script->addGosub('voipdrupal_ntxuva_select_container', $options);
      $script->addGoto('record_message');

      $script->addLabel('select_quarter');
      $beep_url = url('voip/sound/beep', array('absolute' => TRUE));
      $args = array('@beep' => $beep_url);
      $prompt = v('Now type in the number of your quarter after the beep. When done, press the star key. @beep', $args);
      $timeout = 10;
      $end_key = '*';
      $num_digits = 4;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addSet('bairro_name', "^_voipdrupal_ntxuva_validate_quarter(%neighborhood, %input_digits)");

      $script->addGotoIf('set_quarter', "^%bairro_name != ''");
      $script->addSay(v('Invalid option selected. Please try again. '));
      $script->addGoto('select_quarter');

      $script->addLabel('set_quarter');
      $script->addSet('quarter', '%input_digits');
      $script->addSay(v('You selected quarter number: '));
      $script->addSay('%input_digits');
      $script->addSay(v('from'));
      $script->addSay('%bairro_name');

      $script->addLabel('record_message');
      $prompt = v("Please record your report after the beep. When done, press the star key. "); // Please, record your message after the beep. To end, press star.
      $timeout = 5;
      $end_key = '*';
      $max_length = 3600; //1 hour
      $script->addRecord($prompt, $timeout, $end_key, $max_length);

      $script->addLabel('play_menu');
      $prompt = v("To listen to your report, dial 1. To record it once again, dial 2. To accept it, dial 3. "); // To accept, dial 1. To record once again, press 2.
      $timeout = 5;
      $end_key = '';
      $num_digits = 1;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addGotoIf('listen', "^%input_digits == '1'");
      $script->addGotoIf('record_message', "^%input_digits == '2'");
      $script->addGotoIf('accept_recording', "^%input_digits == '3'");

      $script->addSay(v('Invalid input. Please try again.'));
      $script->addGoto('play_menu');

      $script->addLabel('listen');
      $script->addSay('%recording_public_url'); // This is what you recorded:
      $script->addGoto('play_menu');

      $script->addLabel('accept_recording');
      $script->addSay(v("Submitting your report. Please wait. "));
      //@todo: get location?
      $script->addSet('report_number', '^_voipdrupal_ntxuva_create_request (%phone_number, %neighborhood, %container, %quarter, %service_code, %recording_public_url)');
      $script->addGotoIf('report_unsuccessfully', "^%report_number == ''");


      $script->addLabel('report_success');
      $args = array('@report_number' => '%report_number');
      $script->addSay(v("Your report has been submitted. It can be checked with report number @report_number. ", $args));

      /*
      $script->addLabel('receive_via_sms');
      $prompt = v("To receive your report number via text message, dial 1. To go back to the main menu, press the star key. ");
      $timeout = 5;
      $end_key = '';
      $num_digits = 1;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addGotoIf('text_report_id', "^%input_digits == '1'");
      $script->addGotoIf('back_to_main_menu', "^%input_digits == '*'");

      $script->addSay(v('Invalid input. Please try again.'));
      $script->addGoto('receive_via_sms');
      */

      $script->addLabel('text_report_id');
      //@todo: need to be translated through Drupal t()
      $script->addSendText(t('MOPA: your new report id is %report_number'));
      $script->addGoto('go_back');

      $script->addLabel('report_unsuccessfully');
      $script->addSay(v('Processing failure. Could not create request. Please contact the system administrator as soon as possible.'));
      $script->addGoto('go_back');

      $script->addLabel('back_to_main_menu');
      $script->addGosub('voipdrupal_ntxuva_main_menu_script', $options);

      $script->addLabel('go_back');
      $script->addReturn();
      break;
    case 'voipdrupal_ntxuva_select_container':
      $script = new VoipScript('voipdrupal_ntxuva_select_container');
      $voice = $options['voice'];
      voipvoice_set_current_voice($voice);

      $neighborhood = $options['neighborhood'];
      $script->addSet('neighborhood', $neighborhood);
      $containers = voipdrupal_ntxuva_get_containers_for_neighborhood($neighborhood);

      $script->addLabel('select_container');

      $prompt = array();
      $prompt[] = v('Please select the container id. ');
      foreach ($containers as $key => $container) {
        $ivr_key = $key+1;
        $prompt[] = v('For ');
        $prompt[] = v($container['location_name']);
        $prompt[] = v(' dial ');
        $prompt[] = v($ivr_key);
      }
      $timeout = 5;
      $end_key = '';
      $num_digits = 1;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addSet('container_name', "^_voipdrupal_ntxuva_validate_container(%neighborhood, %input_digits)");
      $script->addGotoIf('set_container', "^%container_name != ''");
      $script->addSay(v('Invalid option selected. Please try again. '));
      $script->addGoto('select_container');

      $script->addLabel('set_container');
      $script->addSet('container', '^_voipdrupal_ntxuva_set_container(%neighborhood, %input_digits)');
      $script->addSay(v('You selected container: '));
      $script->addSay('%container_name');

      $script->addReturn();
      break;
    case 'voipdrupal_ntxuva_check_report_script':
      $script = new VoipScript('voipdrupal_ntxuva_check_report_script');
      $voice = $options['voice'];
      voipvoice_set_current_voice($voice);

      $script->addLabel('insert_request_number');
      $beep_url = url('voip/sound/beep', array('absolute' => TRUE));
      $args = array('@beep' => $beep_url);
      $prompt = v("Please type the report number after the beep. Once you are done, type the star key. @beep", $args);
      $timeout = 5;
      $end_key = '*';
      $num_digits = 7;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addSet('request', '%input_digits');
      $script->addSet('request_status', '^_voipdrupal_ntxuva_get_request_status(%request)');

      $script->addGotoIf('request_not_found', "^%request_status == '0'");

      $script->addLabel('request_status');
      $args = array('@report_number' => '%input_digits', '@report_status' => '%request_status');
      $p_options_menu = v('The current status for report @report_number is: @report_status.
To go back to the main menu, dial the star key.', $args);
      $input_options = array(
      '*' => 'back_to_main_menu',
    );
      $p_invalid_msg = v('Invalid input. Please try again.'); // Invalid option, please try again
      $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');

      $script->addLabel('request_not_found');
      $p_options_menu = v('The requested report has not been found. To try again, dial 1. To go back to the main menu, dial the star key.');      $input_options = array(
        '1' => 'insert_request_number',
        '*' => 'back_to_main_menu',
      );
      $p_invalid_msg = v('Invalid input. Please try again.'); // Invalid option, please try again
      $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');

      $script->addLabel('back_to_main_menu');
      $script->addGosub('voipdrupal_ntxuva_main_menu_script', $options);
      break;
  }

  return $script;
}

//_voipdrupal_ntxuva_create_request('+385998844502', 20, 4, 'http://void.media.mit.edu/voipdrupal_ntxuva/sites/default/files/audio_0.mp3');
function _voipdrupal_ntxuva_create_request($phone, $neighborhood_id, $container_id, $quarter_id, $service_code, $recording_public_url) {
  if(isset($container_id)) {
    $location_id = $container_id;
    $type = 'container';
  }
  else {
    $location_id = $quarter_id;
    $type = 'quarter';
  }

  $location_result = voipdrupal_ntxuva_get_location_coordinates($neighborhood_id, $location_id, $type);
  watchdog("voipdrupal_ntxuva", "Location res: " . print_r($location_result, TRUE)); // save request for debugging

  // Validate location
  if (empty($location_result['lat']) || empty($location_result['long'])) {
    return '';
  }

  $lat = $location_result['lat'];
  $long = $location_result['long'];

  // Prepare JSON
  $request = array(
    'phone' => $phone,
    'long' => $long,
    'lat' => $lat,
    'description' => "Gerado por telefone",
    'service_code' => '0' . $service_code,
    'media_url' => $recording_public_url,
  );

  /*if (!empty($location_result['field_address_string']['und'][0]['value'])) {
    $request['address_string'] = $location_result['field_address_string']['und'][0]['value'];
  }*/
  $res = voipdrupal_ntxuva_call_open311('requests', 'POST', $request);
  watchdog("ntxuva_sms", "Report save res: " . print_r($res, TRUE)); // save request for debugging

  // Define reply message
  // If error, send verbose message to user.
  return isset($res[0]['service_request_id']) ? $res[0]['service_request_id'] : '';
}

function _voipdrupal_ntxuva_validate_neighborhood($digit) {
  $digit = (int) $digit;
  if ($digit >= 1 && $digit <= 4) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}
function _voipdrupal_ntxuva_validate_problem_type($digit) {
  $voipdrupal_ntxuva_services = variable_get('voipdrupal_ntxuva_services', array());
  return isset($voipdrupal_ntxuva_services["0".$digit]) ? TRUE: FALSE;
}

function _voipdrupal_ntxuva_problem_type($digit) {
  $voipdrupal_ntxuva_services = variable_get('voipdrupal_ntxuva_services', array());
  return $voipdrupal_ntxuva_services["0".$digit];
}

function _voipdrupal_ntxuva_get_request_status($request) {
  $res = voipdrupal_ntxuva_call_open311("requests/$request.json");

  // If successful, send status
  if (isset($res[0]['service_notice']) && isset($res[0]['service_request_id'])) {
    return $res[0]['service_notice'];
  }
  else {
    return 0;
  }
}

function _voipdrupal_ntxuva_problem_type_needs_container($digit) {
  watchdog('debug', "_voipdrupal_ntxuva_problem_type_needs_container($digit)");

  //Only to be executed if type is 1, 2 or 3
  $digit = (int) $digit;
  if ($digit >= 1 && $digit <= 3) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

function _voipdrupal_ntxuva_validate_container($neighborhood_id, $input_digits) {
  $key = $input_digits-1;
  watchdog('debug', "_voipdrupal_ntxuva_validate_container($neighborhood_id, $input_digits)");
  $containers = voipdrupal_ntxuva_get_containers_for_neighborhood($neighborhood_id);
  return isset($containers[$key]) ? $containers[$key]['location_name'] : '';
}

function _voipdrupal_ntxuva_set_container($neighborhood_id, $input_digits) {
  $key = $input_digits+1;
  $containers = voipdrupal_ntxuva_get_containers_for_neighborhood($neighborhood_id);
  return isset($containers[$key]) ? $containers[$key]['location_id'] : '';
}

function _voipdrupal_ntxuva_validate_quarter($neighborhood_id, $quarter_id) {
  watchdog('debug', "_voipdrupal_ntxuva_validate_quarter($neighborhood_id, $quarter_id)");
  $neighborhood = voipdrupal_ntxuva_get_neighborhood_name($neighborhood_id);
  $quarters = variable_get('voipdrupal_ntxuva_quarters', array());
  return isset($quarters[$neighborhood][$quarter_id]) ? $quarters[$neighborhood][$quarter_id]['location_name'] : '';
}
