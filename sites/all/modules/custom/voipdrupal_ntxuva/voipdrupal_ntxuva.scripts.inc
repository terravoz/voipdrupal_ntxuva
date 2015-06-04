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
      voipvoice_set_current_voice('ntxuva-english');
      $script->addSet('max_session_duration', 0);

      // log the call
      $log_msg = t("Call from %caller_number on @date (id: %call_id)",
        array('@date' => format_date(time(), 'MDT')));
      $script->addLog($log_msg, 'voipdrupal_ntxuva');

      $script->addLabel('welcome');
      $script->addSay(v('Welcome to the MOPA system.'));

      $script->addLabel('language_selection');
      $p_main_menu[] = v("For Portuguese, press one. For Changana, press two. For English, press three. ");
      $input_options = array(
        '1' => 'portuguese_version',
        '2' => 'changana_version',
        '3' => 'english_version',
        'i' => 'invalid_option',
        't' => 'invalid_option'
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

      $script->addLabel('select_problem_type');
      $voipdrupal_ntxuva_services = variable_get('voipdrupal_ntxuva_services', array());
      $prompt = array();
      $prompt[] = v('Please select the type of the problem being reported. ');
      foreach ($voipdrupal_ntxuva_services as $key => $ntxuva_service) {
        $ivr_key = $key{1};
        $prompt[] = v('For ');
        $prompt[] = v($ntxuva_service);
        $prompt[] = v(' dial ');
        $prompt[] = new VoipPrompt($ivr_key);
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
      //@todo: test this!
      //@todo: wrap it in v()
      $script->addSay("^_voipdrupal_ntxuva_problem_type(%input_digits)");

      //Problem type 1,2 and 3 need container id.
      $script->addGotoIf('select_container', "^_voipdrupal_ntxuva_problem_type_needs_container(%input_digits)");
      //Other problem types need neighborhood
      $script->addGoto('select_neighborhood');

      $script->addLabel('select_container');
      $prompt = v("Please type the container id."); // Please, dial the number of the container. To go back to the beginning, dial star.
      $timeout = 5;
      $end_key = '';
      $num_digits = 2;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);
      $script->addSet('location', '%input_digits');

      $script->addSay(v('You selected container number: '));
      $script->addSay('%input_digits');
      //@todo: get category name
      $script->addSay('<category_name>');
      $script->addGoto('record_message');

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

      $script->addLabel('select_quarter');

      $beep_url = url('voip/sound/beep', array('absolute' => TRUE));
      $args = array('@beep' => $beep_url);
      $prompt = v('Now type in the number of your quarter after the beep. When done, press the pound key. @beep', $args);
      $timeout = 10;
      $end_key = '#';
      $num_digits = 99;
      $script->addGetInput($prompt, $num_digits, $end_key, $timeout);

      $script->addGotoIf('no input received', "^%input_digits == ''");
      $script->addGoTo('set_quarter');

      $script->addLabel('no input received');
      $script->addSay(v('Invalid option selected. Please try again. '));
      $script->addGoto('select_quarter');

      $script->addLabel('set_quarter');
      $script->addSet('quarter', '%input_digits');

      //@todo: get location name
      $script->addLabel('record_message');
      //$script->addSet('voipscript_hangup_callback', '_voipdrupal_ntxuva_record_on_hang_up');
      $prompt = v("Please record your report after the beep. When done, press the star key. "); // Please, record your message after the beep. To end, press star.
      $timeout = 5;
      $end_key = '*';
      $max_length = 3600; //1 hour
      $script->addRecord($prompt, $timeout, $end_key, $max_length);

      // reset the hang up handler
      //$script->addSet('voipscript_hangup_callback', '');

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

      $script->addSet('report_number', '^_voipdrupal_ntxuva_create_request (%phone_number, %location, %service_code, %recording_public_url)');
      $script->addGotoIf('report_unsuccessfully', "^%report_number == ''");


      $script->addLabel('report_success');
      $args = array('@report_number' => '%report_number');
      $script->addSay(v("Your report has been submitted. It can be checked with report number @report_number. ", $args));

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

      $script->addLabel('text_report_id');
      //@todo: need to be translated through Drupal t()
      $script->addSendText(t('Report number: %report_number'));
      $script->addGoto('go_back');

      $script->addLabel('report_unsuccessfully');
      $script->addSay(v('Processing failure. Could not create request. Please contact the system administrator as soon as possible.'));
      $script->addGoto('go_back');

      $script->addLabel('back_to_main_menu');
      $script->addGosub('voipdrupal_ntxuva_main_menu_script', $options);

      $script->addLabel('go_back');
      $script->addReturn();
      break;
    case 'voipdrupal_ntxuva_check_report_script':
      $script = new VoipScript('voipdrupal_ntxuva_check_report_script');
      $voice = $options['voice'];
      voipvoice_set_current_voice($voice);

      $script->addLabel('insert_request_number');
      $beep_url = url('voip/sound/beep', array('absolute' => TRUE));
      $args = array('@beep' => $beep_url);
      $prompt = v("Please type the report number after the beep. Once you are done, type the pound key. @beep", $args);
      $timeout = 5;
      $end_key = '#';
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
      '*' => 'go_back',
    );
      $p_invalid_msg = v('Invalid input. Please try again.'); // Invalid option, please try again
      $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);

      $script->addLabel('request_not_found');
      $p_options_menu = v('The requested report has not been found. To try again, dial 1. To go back to the main menu, dial the star key.');      $input_options = array(
        '1' => 'insert_request_number',
        '*' => 'go_back',
      );
      $p_invalid_msg = v('Invalid input. Please try again.'); // Invalid option, please try again
      $script->addRunIvrMenu($p_options_menu, $input_options, $p_invalid_msg);
      $script->addGoto('%ivr_option_selected');

      $script->addLabel('go_back');
      $script->addReturn();
      break;
  }

  return $script;
}

//_voipdrupal_ntxuva_create_request('+385998844502', 20, 4, 'http://void.media.mit.edu/voipdrupal_ntxuva/sites/default/files/audio_0.mp3');
function _voipdrupal_ntxuva_create_request($phone, $location, $service_code, $recording_public_url) {

  //@todo: get location
  $location_result = voipdrupal_ntxuva_garbage_spots_by_name('0' . $location);

  watchdog("ntxuva_sms", "Location 1: " . print_r($location, TRUE)); // save request for debugging
  watchdog("ntxuva_sms", "Location 2: " . print_r($location_result, TRUE)); // save request for debugging

  // Validate location
  if (empty($location_result)) {
    return '';
  }

  $lat = $location_result['field_lat']['und'][0]['value'];
  $long = $location_result['field_long']['und'][0]['value'];

  // Prepare JSON
  $request = array(
    'phone' => $phone,
    'long' => $long,
    'lat' => $lat,
    'description' => "Gerado por telefone",
    'service_code' => '0' . $service_code,
    'media_url' => $recording_public_url,
  );

  if (!empty($location_result['field_address_string']['und'][0]['value'])) {
    $request['address_string'] = $location_result['field_address_string']['und'][0]['value'];
  }
  $res = voipdrupal_ntxuva_call_open311('requests', 'POST', $request);

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
  $part = str_split($request, 4);
  $res = voipdrupal_ntxuva_call_open311("requests/{$part[0]}-{$part[1]}.json");

  // If successful, send status
  if (isset($res[0]['status'])) {
    return $res[0]['status'];
  }
  else {
    return 0;
  }
}

function _voipdrupal_ntxuva_problem_type_needs_container($digit) {
  //Only to be executed if type is 1, 2 or 3
  $digit = (int) $digit;
  if ($digit >= 1 && $digit <= 3) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}