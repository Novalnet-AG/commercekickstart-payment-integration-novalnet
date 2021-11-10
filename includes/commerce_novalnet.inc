<?php

 /**
  * @file
  * Novalnet payment method module
  * This module is used for real time processing of
  * Novalnet transaction of customers.
  *
  * @category   PHP
  * @package    commerce_novalnet
  * @author     Novalnet AG
  * @copyright  Copyright by Novalnet
  * @license    https://www.novalnet.com/payment-plugins/free/license
  * @version    11.1.2
  *
  *  Script : commerce_novalnet.inc
  */

/**
 * Include commerce_novalnet.form.inc file.
 */
  module_load_include('inc', 'commerce_novalnet', 'includes/commerce_novalnet.form');

// Define paygate URLs on constants.
define('COMMERCE_NOVALNET_PAYGATE_URL', 'https://payport.novalnet.de/paygate.jsp');
define('COMMERCE_NOVALNET_XML_URL', 'https://payport.novalnet.de/nn_infoport.xml');
define('COMMERCE_NOVALNET_AUTO_CONFIG', 'https://payport.novalnet.de/autoconfig');
define('NOVALNET_IMAGES_PATH', base_path() . drupal_get_path('module', 'commerce_novalnet') . '/images/');

/**
 * Function for validate the element value as numeric.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_number($element, &$form_state) {
  $value = trim($element['#value']);
  if ($value != '') {
    if (!preg_match("/^[0-9]+$/", $value)) {
      form_error($element, t('Invalid @title', array('@title' => $element['#title'])));
    }
  }
}

/**
 * Function for validate the element manimum amount for guarantee payment.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_guarantee_amount($element, &$form_state) {
  $value = trim($element['#value']);
  if (strpos($element['#name'], 'min_amount') !== FALSE) {    
    if ($value != '') {
      if (!preg_match("/^[0-9]+$/", $value) || $value < 2000) {
        form_error($element, t('Minimum amount should be greater than or equal to 20,00 EUR'));
      }
    }
  }
}

/**
 * Function for validate the payment due date.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_due_date($element, &$form_state) {
  $value = trim($element['#value']);
  if ($value != '') {
    $min_due_date = (isset($form_state['element_settings']['payment_method']['method_id']) && $form_state['element_settings']['payment_method']['method_id'] == 'commerce_novalnet_invoice') ? 0 : 7;
    if (!preg_match("/^[0-9]+$/", $value) || $value < $min_due_date) {
      form_error($element, t('Invalid @title', array('@title' => $element['#title'])));
    }
  }
}

/**
 * Function for get merchant details from Novalnet server.
 *
 * @param string $process_key
 *
 * @return array
 */
function commerce_novalnet_get_merchant_details($process_key) {
  $process_key = trim($process_key);
  if (!empty($process_key)) {
    global $language;
    $data = array(
      'system_ip'       => commerce_novalnet_get_req_ip_address('SERVER_ADDR'),
      'api_config_hash' => $process_key,
      'lang'            => '',
    );
    // Call this function for send request to Novalnet server.
    $response = drupal_json_decode(commerce_novalnet_drupal_request($data, COMMERCE_NOVALNET_AUTO_CONFIG));
    $json_error = json_last_error();

    // If the response doesn't have any error.
    if (empty($json_error)) {

      if (isset($response['config_result'])) {
        return array('status' => '90', 'status_desc' => $response['config_result']);
      }

      $merchant_details = array(
        'status'     => 100,
        'public_key' => $process_key,
        'vendor_id'  => $response['vendor_id'],
        'auth_code'  => $response['auth_code'],
        'access_key' => $response['access_key'],
        'test_mode'  => $response['test_mode'],
        'product_id' => $response['product_id'],
      );
      $tariff_name      = preg_replace('/tariff_name\d+:/', '', explode(',', $response['tariff_name']));
      $tariff_id        = preg_replace('/tariff_id\d+:/', '', explode(',', $response['tariff_id']));
      $tariff_type      = preg_replace('/tariff_type\d+:/', '', explode(',', $response['tariff_type']));
      $available_tariff = array_combine($tariff_id, $tariff_name);
      $subs_tariff      = array_intersect_key($tariff_id, array_intersect($tariff_type, array(1, 4)));
      $merchant_details['normal_tariff']       = array_map('utf8_decode', array_diff_key($available_tariff, array_flip($subs_tariff)));
      $merchant_details['subscription_tariff'] = array_diff($available_tariff, $merchant_details['normal_tariff']);
      return $merchant_details;
    }
    return array('status' => '90', 'status_desc' => t('Invalid @title', array('@title' => t('Product activation key'))));
  }
  return array('status' => '90', 'status_desc' => t('Invalid @title', array('@title' => t('Product activation key'))));
}

/**
 * Function for perform HTTP requests.
 *
 * @param array $data
 * @param string $host
 * @param bool $xml
 */
function commerce_novalnet_drupal_request($data, $host, $xml = FALSE) {
  $data['remote_ip'] = commerce_novalnet_get_req_ip_address('REMOTE_ADDR');
  if (isset($data['email'])) {
    $customer_data = array($data['email'], $data['first_name'], $data['last_name'], $data['city'], $data['street'], $data['zip'], $data['country_code']);
    if (count(array_filter($customer_data)) != count($customer_data)) {
      return 'status=404&status_desc=' . t('Please fill all the user information to proceed the payment');
    }
  }
  if (isset($data['payment_access_key'])) {
    unset($data['payment_access_key']);
  }
  if ($xml) {
    $request = '<?xml version="1.0" encoding="UTF-8"?>
      <nnxml>
        <info_request>';
    foreach ($data as $tag => $val) {
      $request .= '<' . $tag . '>' . $val . '</' . $tag . '>';
    }
    $request .= '</info_request>
        </nnxml>';
  }
  else {
    $request = http_build_query($data);
  }

  $options = array(
    'headers' => array('Content-Type' => 'application/x-www-form-urlencoded'),
    'method' => 'POST',
    'data' => $request,
  );

  $global_setting = variable_get('novalnet_global_settings', '');
  if (!empty($global_setting['gateway_timeout'])) {
      $options['timeout'] = $global_setting['gateway_timeout'];
  }

  $response = drupal_http_request($host, $options);

  if (!isset($response->data)) {
    $response->data = 'status=404&status_desc=' . $response->error;
    if ($xml) {
      $response->data = '<nnxml><status>404</status><status_message>' . $response->error . '</status_message></nnxml>';
    }
  }
  parse_str($response->data, $parsed_response);
  return $response->data;
}

/**
 * Function for validate basic merchant details.
 *
 * @param array $payment_method
 * @param array $order_id
 *
 * @return array
 */
function commerce_novalnet_validate_basic_data($payment_method, $order_id = '') {
  $global_setting = variable_get('novalnet_global_settings', '');
  if (empty($global_setting)) {
    return FALSE;
  }
  if (empty($order_id)) {
    $order_id = (arg(0) == 'admin') ? arg(3) : arg(1);
  }

  if (empty($order_id) || !is_numeric($order_id)) {
    return $global_setting;
  }

  $order = commerce_order_load($order_id);
  if (module_exists('commerce_recurring') && !empty($global_setting['subscription']) && !empty($order->commerce_line_items)) {
    // Validate the recurring details(Only if the shop has 'commerce_recurring' module)
    return commerce_novalnet_validate_recurring_data($order, $global_setting, $payment_method);
  }
  return $global_setting;
};

/**
 * Function for validate order for displaying subscription payment.
 *
 * @param object $order
 * @param array $global_setting
 * @param array $payment_method
 *
 * @return array / bool
 */
function commerce_novalnet_validate_recurring_data($order, $global_setting, $payment_method) {
  $recurring_product = FALSE;
  foreach ($order->commerce_line_items['und'] as $line_id) {
    $line_item = commerce_line_item_load($line_id['line_item_id']);
    if ($line_item->type == 'product') {
      $product   = commerce_product_load($line_item->commerce_product['und'][0]['product_id']);
      if (commerce_recurring_product_is_recurring($product)) {
        $recurring_product = TRUE;
        $global_setting['subs_product'][] = array(
          'product_id' => $product->product_id,
          'quantity'      => $line_item->quantity,
        );
      }
    }
  }

  if ($recurring_product) {
    if (!in_array($payment_method, $global_setting['subs']['subscription_payments'])) {
      return FALSE;
    }
    $global_setting['tariff_id'] = $global_setting['subs']['subs_tariff'];
  }

  return $global_setting;
}

/**
 * Function for getting reference TID.
 *
 * @param int $uid
 * @param string $payment_method
 *
 * @return int or bool
 */
function commerce_novalnet_reference_tid($uid, $payment_method) {
  global $user;
  if ($uid <= 0 || $user->uid != $uid) {
    return array();
  }
  $db_result = db_select('commerce_novalnet_transaction_detail', 'tid')
    ->fields('tid', array('tid', 'payment_details'))
    ->condition('customer_id', $uid)
    ->condition('payment_type', $payment_method)
    ->condition('payment_ref', 0)
    ->condition('one_click_shopping', '1')
    ->orderBy('id', 'DESC')
    ->execute()
    ->fetchAssoc();
  if (empty($db_result)) {
    return FALSE;
  }
  return $db_result;
}

/**
 * Function for generate unique string.
 *
 * @return string
 */
function commerce_novalnet_get_random_string() {
  $random_chars = array('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0');
  shuffle($random_chars);
  return substr(implode("", $random_chars), 0, 30);
}

/**
 * Function for build common request array for all payments.
 *
 * @param array $data
 * @param array $payment_method
 * @param object $order
 * @param array $charge
 */
function commerce_novalnet_get_common_params(&$data, $payment_method, $order, $charge = array()) {
  global $language;
  $lang   = strtoupper($language->language);
  $charge = (empty($charge)) ? $order->commerce_order_total['und']['0'] : $charge;
  $global_setting   = $payment_method['global_setting'];
  $payment_settings = $payment_method['settings'];
  $billing_wrapper  = entity_metadata_wrapper('commerce_order', $order);
  $billing_details  = $billing_wrapper->commerce_customer_billing->commerce_customer_address->value();
  $novalnet_info    = system_get_info('module', 'commerce_novalnet');
  $commerce_info    = system_get_info('module', 'commerce');

  if (empty($billing_details['first_name']) && empty($billing_details['last_name'])) {
    $name_line                     = explode(' ', $billing_details['name_line']);
    $billing_details['first_name'] = $name_line[0];
    $billing_details['last_name']  = ltrim($billing_details['name_line'], $billing_details['first_name'] . ' ');
  }
  $params = array(
    'vendor'             => $global_setting['vendor_id'],
    'product'            => $global_setting['product_id'],
    'tariff'             => $global_setting['tariff_id'],
    'auth_code'          => $global_setting['auth_code'],
    'payment_type'       => constant(strtoupper($payment_method['method_id'])),
    'key'                => constant(strtoupper($payment_method['method_id']) . '_KEY'),
    'payment_method'     => $payment_method['method_id'],
    'test_mode'          => isset($payment_settings['test_mode']) ? $payment_settings['test_mode'] : 0,
    'currency'           => $charge['currency_code'],
    'first_name'         => $billing_details['first_name'],
    'last_name'          => empty($billing_details['last_name']) ? $billing_details['first_name'] : $billing_details['last_name'],
    'gender'             => 'u',
    'email'              => $order->mail,
    'street'             => trim($billing_details['thoroughfare'] . ' ' . $billing_details['premise']),
    'search_in_street'   => 1,
    'city'               => $billing_details['locality'],
    'zip'                => $billing_details['postal_code'],
    'lang'               => $lang,
    'language'           => $lang,
    'country'            => $billing_details['country'],
    'country_code'       => $billing_details['country'],
    'session'            => session_id(),
    'order_no'           => $order->order_id,
    'customer_no'        => $order->uid,
    'system_name'        => 'drupal-commerce',
    'system_version'     => $commerce_info['version'] . '-NN' . $novalnet_info['version'],
    'system_ip'          => commerce_novalnet_get_req_ip_address('SERVER_ADDR'),
    'system_url'         => url('', array('absolute' => TRUE)),
    'amount'             => $charge['amount'],
  );

  if (!in_array($params, array('27', '37', '40', '41'))) {
    $params['payment_access_key'] = $global_setting['payment_access_key'];
  }

  $data = array_merge($params, $data);
  // Call this function for check affiliate order.
  commerce_novalnet_check_affiliate_order($data);
  commerce_novalnet_add_recurring_data($data, $global_setting);
  // Call this function for check on-hold payment.
  commerce_novalnet_check_onhold($data, $global_setting, $payment_settings);
  $reference1 = trim(strip_tags($payment_settings['reference1']));
  if ($reference1 != '') {
    $data['input1']     = 'reference1';
    $data['inputval1']  = $reference1;
  }

  $reference2 = trim(strip_tags($payment_settings['reference2']));
  if ($reference2 != '') {
    $data['input2']     = 'reference2';
    $data['inputval2']  = $reference2;
  }

  if (!empty($billing_details['organisation_name'])) {
    $data['company']     = $billing_details['organisation_name'];
  }

  $notify_url = trim($global_setting['callback_notify_url']);
  if (!empty($notify_url)) {
    $data['notify_url'] = $notify_url;
  }

  $referrer_id = trim($global_setting['referrer_id']);
  if (!empty($referrer_id)) {
    $data['referrer_id'] = trim($referrer_id);
  }
}

/**
 * Function for check on-hold transaction by using manual check amount.
 *
 * @param array $data
 * @param array $global_setting
 * @param array $payment_settings
 */
function commerce_novalnet_check_onhold(&$data, $global_setting, $payment_settings) {
  if (!in_array($data['payment_method'], array('commerce_novalnet_cc', 'commerce_novalnet_sepa', 'commerce_novalnet_invoice', 'commerce_novalnet_paypal')) || (isset($payment_settings['shopping_type']) && $payment_settings['shopping_type'] == 'zero_amount' && empty($data['tariff_period']))) {
    return TRUE;
  }

  // If the merchant have configure the manual check amount and the amount is lesser than order amount then add on_hold param.
  if (!empty($payment_settings['manual_amount']) && $data['amount'] >= $payment_settings['manual_amount']) {
    $data['on_hold'] = 1;
  }
}

/**
 * Function for check affiliate order.
 *
 * @param array $data
 */
function commerce_novalnet_check_affiliate_order(&$data) {
  // Call this function for get Affiliate Id from the affiliate table.
  $aff_id = !empty($_SESSION['nn_aff_id']) ? $_SESSION['nn_aff_id'] : commerce_novalnet_get_affiliate_details($data);
  if (empty($aff_id)) {
    return TRUE;
  }

  // Call this function for get Affiliate details from the affiliate table.
  $aff_details = commerce_novalnet_get_affiliate_details($data, $aff_id);
  if (empty($aff_details)) {
    return TRUE;
  }

  if (isset($data['vendor_id'])) {
    $data['vendor_id']             = $_SESSION['nn_aff_id'];
  }
  else {
    $data['vendor']           = $_SESSION['nn_aff_id'];
  }

  $data['auth_code']          = $aff_details['aff_authcode'];
  $data['payment_access_key'] = $aff_details['aff_accesskey'];

}

/**
 * Function for get affiliate id from transaction table.
 *
 * @param array $data
 * @param int $aff_id
 *
 * @return bool / array
 */
function commerce_novalnet_get_affiliate_details($data, $aff_id = FALSE) {
  if (!$aff_id) {
    if ($data['customer_no'] > 0) {
      $db_result = db_select('commerce_novalnet_transaction_detail', 'affiliate')
        ->fields('affiliate', array('affiliate'))
        ->condition('customer_id', $data['customer_no'])
        ->orderBy('id', 'DESC')
        ->execute()
        ->fetchAssoc();
      if (empty($db_result)) {
        return FALSE;
      }
      return $_SESSION['nn_aff_id'] = $db_result['affiliate'];
    }
    return FALSE;
  }

  $db_result = db_select('commerce_novalnet_affiliate_detail', 'aff_id')
    ->fields('aff_id', array('aff_authcode', 'aff_accesskey'))
    ->condition('aff_id', $aff_id)
    ->orderBy('id', 'DESC')
    ->execute()
    ->fetchAssoc();
  if (empty($db_result)) {
    return FALSE;
  }
  return $db_result;
}

/**
 * Function for generate transaction comments.
 *
 * @param array $response
 * @param array $payment
 * @param bool $rec_order
 *   return string.
 */
function commerce_novalnet_get_comments($response, $payment, $rec_order = '') {

  $trans_array = array(
    '@test'   => (!empty($response['test_mode']) || !empty($payment['settings']['test_mode'])) ? t('Test order') : '',
    '@payment_name'  => $payment['title'],
    '@tid'   => $response['tid'],
  );
  $additional_comments = '';
  if (in_array($payment['method_id'], array('commerce_novalnet_invoice', 'commerce_novalnet_prepayment'))) {
    // Call this function for getting added message for invoice/prepayment payment method.
    if($response['tid_status'] == 75 && $response['key'] == 41) {
		$additional_comments = '<br> <div id="guarantee_hold" >' . t('Your order is under verification and once confirmed, we will send you our bank details to where the order amount should be transferred. Please note that this may take upto 24 hours.') . '</div>';		
	} 
	
	if($response['tid_status'] == 100) {
		$additional_comments = commerce_novalnet_get_invoice_prepayment_message($response, $payment, $rec_order);
	}
  }
  elseif ($payment['method_id'] == 'commerce_novalnet_barzahlen') {
      $additional_comments = commerce_novalnet_get_barzahlen_comments($response);
  }
  $gurantee_info = '';
  if (in_array($response['key'], array(40, 41))) {
    $gurantee_info = t('This is processed as a guarantee payment');
  }

  return $gurantee_info . '<br>' . t('@test<br>Payment Method: @payment_name<br>Novalnet transaction ID: @tid', $trans_array) . '<br>' . $additional_comments;
}

/**
 * Function for get transaction message for Invoice and Prepayment.
 *
 * @param array $response
 *   return string.
 */
function commerce_novalnet_get_barzahlen_comments($response) {
  $strnos = 0;
  foreach ($response as $key => $val) {
    if (strpos($key, 'nearest_store_title_') !== FALSE) {
      $strnos++;
    }
  }
  $shops = t('Store(s) near you:') . '<br>';
  for ($i = 1; $i <= $strnos; $i++) {
    $shops .= $response['nearest_store_title_' . $i] . ': ' . 
              ( isset($response['nearest_store_street_' . $i])? $response['nearest_store_street_' . $i] . ', ' : '' ) .
              ( isset($response['nearest_store_city_' . $i])? $response['nearest_store_city_' . $i] . ', ' : '' ) .
              ( isset($response['nearest_store_country_' . $i])? $response['nearest_store_country_' . $i] . ', ' : '') .
              ( isset($response['nearest_store_zipcode_' . $i])? $response['nearest_store_zipcode_' . $i] : '' ) . '<br>';
  }
  return t('Slip expiry date') . ': <span id="due_date">' . date('m/d/Y', strtotime($response['cashpayment_due_date'])) . '</span><br>' . $shops ;
}
/**
 * Function for get transaction message for Invoice and Prepayment.
 *
 * @param array $response
 * @param array $payment
 * @param bool $rec_order
 *   return string.
 */
function commerce_novalnet_get_invoice_prepayment_message($response, $payment, $rec_order = '') {
	
	
  $trans_details   = commerce_novalnet_order_merchant_details($response['order_no'], $response['tid']);
  $payment_details = !empty($trans_details['payment_details']) ? unserialize($trans_details['payment_details']) : '';
  
  $account_info  = '<br>' . t('Please transfer the amount to the below mentioned account details of our payment processor Novalnet') . '<br>';
  $account_info .= !empty($response['due_date']) ? t('Due date') . ': <span id="due_date">' . date('m/d/Y', strtotime($response['due_date'])) .  '</span><br>' : '';
  $account_info .= t('Account holder') . ': ' . $response['invoice_account_holder'] . '<br>' . t('IBAN') . ': ' . $response['invoice_iban'] . '<br>' . t('BIC') . ': ' . $response['invoice_bic'] . '<br>' . t('Bank') . ': ' . $response['invoice_bankname'] . ' ' . $response['invoice_bankplace'] . '<br>' . t('Amount') . ': <span id="amount">' . commerce_currency_format($response['amount'], $response['currency']) . '</span><br><br>';

  if (!empty($rec_order)) {
    return '<div id="prepayment_comments">' . $account_info . '</div>';
  }

  $references_settings = $payment['settings']['payment_references'];
  $response['invoice_ref'] = isset($response['invoice_ref']) ? $response['invoice_ref'] : $payment_details['invoice_ref'];

  $payment_ref_array = array_filter(
    array(
      $response['invoice_ref']                        => $references_settings['payment_ref_1'],
      t('TID') . ' ' . $response['tid']               => $references_settings['payment_ref_2'],
      t('Order number') . ' ' . $response['order_no'] => $references_settings['payment_ref_3'],
    )
  );

  $ref_count = count($payment_ref_array);
  $ref_info = (($ref_count > 1) ? t('Please use any one of the following references as the payment reference, as only through this way your payment is matched and assigned to the order:') : t('Please use the following payment reference for your money transfer, as only through this way your payment is matched and assigned to the order:')) . '<br>';
  $i = 1;

  foreach ($payment_ref_array as $key => $val) {
    $ref_info .= (($ref_count > 1) ? t('Payment Reference @no', array('@no' => $i)) : t('Payment Reference')) . ': ' . $key . '<br>';
    $i++;
  }
  return '<div id="prepayment_comments">' . $account_info . $ref_info . '</div>';
}

/**
 * Function for complete the order.
 *
 * @params object $order
 * @params array $response
 * @params array $payment_method
 * @params bool $rec_order
 */
function commerce_novalnet_complete_order($order, $response, $payment_method, $rec_order = '') {
  $response['amount'] = str_replace('.', '', $response['amount']);
  // Call this function for get transaction comments.
  $trans_message = commerce_novalnet_get_comments($response, $payment_method, $rec_order);
  $response['paid_until'] = !empty($response['next_subs_cycle']) ? $response['next_subs_cycle'] : (!empty($response['paid_until']) ? $response['paid_until'] : '');
  $response['paid_until'] = !empty($response['paid_until']) ? date('m/d/Y', strtotime($response['paid_until'])) : '';

  if (!empty($rec_order) && $rec_order != 'communication_failure') {
    global $base_url;
    $recurring_entity = commerce_recurring_load_by_order($order);
    $recurring_entity[0]->due_date = strtotime($response['paid_until']);
    entity_save('commerce_recurring', $recurring_entity[0]);

    $order_link = $base_url . '/user/' . $order->uid . '/orders/' . $rec_order;
    $trans_message .= t("Next charging date : @ndate <br>Reference Order : <a href='@order_link'>@order_no</a>", array('@order_no' => $rec_order, '@order_link' => $order_link, '@ndate' => $response['paid_until']));
  }

  $response['amount']       = in_array($payment_method['method_id'], array('commerce_novalnet_invoice', 'commerce_novalnet_prepayment', 'commerce_novalnet_barzahlen')) ? 0 : $response['amount'];
  $response['order_status'] = $payment_method['settings']['order_status'];

  if($response['key'] == 41 && $response['tid_status'] == 75) {
	$response['order_status'] = $payment_method['settings']['guarantee_payment']['guarantee_pending_status'];
    $response['amount']       = $response['amount'];
  }
  elseif ($response['key'] == 41) {
    $response['order_status'] = $payment_method['settings']['callback_status'];
    $response['amount']       = $response['amount'];
  }
  
  if($response['key'] == 40 && $response['tid_status'] == 75) {
	  $response['order_status'] = $payment_method['settings']['guarantee_payment']['guarantee_pending_status'];
  }

  $response['org_amount'] = $order->commerce_order_total['und']['0']['amount'];


  if (in_array($response['tid_status'], array(85, 86, 90))) {
    if ($response['tid_status'] != 85)
		$response['order_status'] = ($payment_method['method_id'] == 'commerce_novalnet_paypal') ? $payment_method['settings']['paypal_pending_status'] : $payment_method['settings']['przelewy24_pending_status'];
    $response['amount']       = 0;
  }

  // Store Transaction details to the commerce_novalnet_transaction_detail table.
  commerce_novalnet_store_transaction($response, $payment_method, $rec_order);

  commerce_novalnet_create_new_transaction(array(
    'order_id'            => $order->order_id,
    'amount'              => $response['amount'],
    'currency'            => $response['currency'],
    'tid'                 => $response['tid'],
    'trans_status'        => $response['tid_status'],
    'reference_id'        => !empty($response['payment_ref']) ? $response['payment_ref'] : NULL,
    'payment_id'          => $payment_method['method_id'],
    'instance_id'         => $payment_method['instance_id'],
    'message'             => array('@message' => $trans_message),
  // Call this function for creating transaction for the particular order in shop.
  ));
  if ($response['key'] == 59) {
    $order->data['checkout_token'] = $response['cp_checkout_token'];
  }
  $order->data['test_mode'] = $response['test_mode'];
  $order->data['tid'] = $response['tid'];
  $order->data['transaction_details'][] = array('update_on' => time(), 'message' => $trans_message);

  if (!empty($response['subs_id'])) {
    $order->data['subs'] = array(
      'subs_id'         => $response['subs_id'],
      'next_cycle_date' => strtotime($response['paid_until']),
    );
  }

  commerce_checkout_complete($order);
  if (isset($order->data['payment_data'])) {
    unset($order->data['payment_data']);
  }
  commerce_order_save($order);

  // Core function for update the order status.
  commerce_order_status_update($order, $response['order_status'], FALSE, TRUE, t('Order status updated'));

  $global_setting = !empty($payment_method['global_setting']['test_notify']) ? $payment_method['global_setting'] : variable_get('novalnet_global_settings');
  if (!empty($response['test_mode']) && empty($payment_method['settings']['test_mode']) && !empty($global_setting['test_notify'])) {
    commerce_novalnet_send_test_transaction_notification($order->order_id);
  }

  commerce_novalnet_session();
  if (!empty($rec_order)) {
    return array('order_id' => $order->order_id, 'trans_message' => $trans_message);
  }
  if (arg(0) == 'admin') {
    $order->data['payment_method'] = $payment_method['method_id'] . '|commerce_payment_' . $payment_method['method_id'];
    commerce_order_save($order);
    drupal_goto('admin/commerce/orders/' . $order->order_id . '/payment');
  }
  drupal_goto('checkout/' . $order->order_id . '/complete');
}

/**
 * Create transaction.
 *
 * @param array $trans_details
 */
function commerce_novalnet_create_new_transaction($trans_details) {
  $transaction = commerce_payment_transaction_new($trans_details['payment_id'], $trans_details['order_id']);
  $transaction->instance_id       = $trans_details['instance_id'];
  $transaction->amount            = $trans_details['amount'];
  $transaction->currency_code     = $trans_details['currency'];
  $transaction->status            = (!empty($trans_details['failure']) ? COMMERCE_PAYMENT_STATUS_FAILURE : (($trans_details['trans_status'] != 100) ? COMMERCE_PAYMENT_STATUS_PENDING : COMMERCE_PAYMENT_STATUS_SUCCESS));
  $transaction->message           = '@message';
  $transaction->message_variables = str_replace('<br>', "\n", $trans_details['message']);
  if (!empty($trans_details['reference_id'])) {
    $transaction->payload           = t('Reference Transaction: @reference', array('@reference' => $trans_details['reference_id']));
  }
  $transaction->remote_id         = $trans_details['tid'];
  $transaction->remote_status     = $trans_details['trans_status'];
  commerce_payment_transaction_save($transaction);
}

/**
 * Function for send test transaction notification mail.
 *
 * @param int order_id
 */
function commerce_novalnet_send_test_transaction_notification($order_id) {
  $params = array(
    'body'    => t('Dear client,<br> We would like to inform you that test order @order_id has been placed in your shop recently.Please make sure your project is in LIVE mode at Novalnet administration portal and Novalnet payments are enabled in your shop system. Please ignore this email if the order has been placed by you for testing purpose.<br><br>Regards,<br>Novalnet AG', array('@order_id' => $order_id)),
    'subject' => t('Novalnet test order notification - @shop', array('@shop' => variable_get('site_name'))),
    'headers' => array(),
  );
  drupal_mail('commerce_novalnet', '', variable_get('site_mail'), '', $params, variable_get('site_mail'), TRUE);
}

/**
 * Implements hook_i18n_string_list().
 */
function commerce_novalnet_i18n_string_list($group) {
  if ($group == 'commerce') {
    $global_setting = variable_get('novalnet_global_settings', '');
    $callback_setting = variable_get('novalnet_callback_settings', '');
    $strings['commerce']['novalnet']['test_trans']['mail_body'] = $global_setting['test_trans']['mail_body'];
    $strings['commerce']['novalnet']['test_trans']['mail_subject'] = $global_setting['test_trans']['mail_subject'];
    $strings['commerce']['novalnet']['callback']['mail_body'] = $callback_setting['callback_mail_settings']['callback_mail_subject'];
    $strings['commerce']['novalnet']['callback']['mail_subject'] = $callback_setting['callback_mail_settings']['callback_mail_body'];
    return $strings;
  }
}

/**
 * Function for store transaction details on commerce_novalnet_transaction_detail table.
 *
 * @param array $data,
 * @param array $payment_method
 * @param bool $rec_order
 *
 * @return bool
 */
function commerce_novalnet_store_transaction($data, $payment_method, $rec_order = FALSE) {
  $vendor_details = serialize(array(
    'vendor'    => $data['vendor'],
    'product'   => $data['product'],
    'tariff'    => $data['tariff'],
    'auth_code' => $data['auth_code'],
  ));
  $booked      = 1;
  switch ($payment_method['method_id']) {
    case 'commerce_novalnet_cc':
      $payment_details = array(
        'cc_holder'    => $data['cc_holder'],
        'cc_no'        => $data['cc_no'],
        'cc_exp_year'  => $data['cc_exp_year'],
        'cc_exp_month' => $data['cc_exp_month'],
        'cc_card_type' => !empty($data['cc_card_type']) ? $data['cc_card_type'] : '',
      );
      $booked = ($payment_method['settings']['shopping_type'] == 'zero_amount' && $data['amount'] == 0) ? 0 : 1;

      break;

    case 'commerce_novalnet_sepa':

      if (isset($data['iban'])) {

        $payment_details = array(
          'iban'                => $data['iban'],
          'bic'                 => $data['bic'],
          'bankaccount_holder'  => $data['bankaccount_holder'],
        );
      }

      if (isset($data['sepa_hash'])) {
        $payment_details['sepa_hash']  = $data['sepa_hash'];
      }

      $booked = ($payment_method['settings']['shopping_type'] == 'zero_amount' && $data['amount'] == 0) ? 0 : 1;

      break;

    case 'commerce_novalnet_invoice':
    case 'commerce_novalnet_prepayment':
      $payment_details = array(
        'due_date'          => isset($data['due_date']) ? $data['due_date'] : '',
        'invoice_iban'      => $data['invoice_iban'],
        'invoice_bic'       => $data['invoice_bic'],
        'invoice_bankname'  => $data['invoice_bankname'],
        'invoice_bankplace' => $data['invoice_bankplace'],
        'invoice_ref'       => $data['invoice_ref'],
      );
      break;

    case 'commerce_novalnet_paypal':
      if (isset($data['paypal_transaction_id'])) {
        $payment_details = array('paypal_transaction_id' => $data['paypal_transaction_id']);
      }
      $booked = ($payment_method['settings']['shopping_type'] == 'zero_amount' && $data['amount'] == 0) ? 0 : 1;
      break;
    case 'commerce_novalnet_barzahlen':
      if (isset($data['cashpayment_due_date'])) {
        $payment_details = array('due_date' => $data['cashpayment_due_date']);
      }
      break;
  }

  if (isset($data['birth_date'])) {
    $payment_details['birth_date']   = $data['birth_date'];
  }

  $payment_details['payment_type'] = $data['payment_type'];

  $db_array = array(
    'tid'                => $data['tid'],
    'vendor_details'     => $vendor_details,
    'status'             => $data['tid_status'],
    'payment_type'       => $payment_method['method_id'],
    'payment_key'        => $data['key'],
    'paid_amount'        => (int) $data['amount'],
    'customer_id'        => empty($data['customer_no']) ? 0 : $data['customer_no'],
    'order_id'           => $data['order_no'],
    'subs_id'            => !empty($data['subs_id']) ? $data['subs_id'] : 0,
    'affiliate'          => $data['vendor'],
    'changed'            => time(),
    'org_total'          => $data['org_amount'],
    'refunded_amount'    => 0,
    'booked'             => $booked,
    'payment_ref'        => (!empty($data['payment_ref'])) ? $data['payment_ref'] : 0,
    'one_click_shopping' => (isset($payment_method['settings']['shopping_type']) && $payment_method['settings']['shopping_type'] == 'one_click' && empty($rec_order)) ? 1 : 0,
    'payment_details'    => serialize($payment_details),
  );
  db_insert('commerce_novalnet_transaction_detail')->fields($db_array)->execute();
  return TRUE;
}

/**
 * Function for encode the params.
 *
 * @param array $data
 *   return encoded string.
 */
function commerce_novalnet_encode_params(&$data) {
  $encode_params = array('product', 'tariff', 'test_mode', 'uniqid', 'amount', 'auth_code');
  foreach ($encode_params as $element) {
    try {
      $crc = sprintf('%u', crc32($data[$element]));
      $data[$element] = strrev(base64_encode(bin2hex($crc . "|" . $data[$element] . $data['payment_access_key'])));
    }
    catch (Exception $e) {
      drupal_set_message(check_plain($e), 'error');
      return FALSE;
    }
  }

  // Call this function for create hash.
  $data['hash'] = commerce_novalnet_create_hash($data, $data['payment_access_key']);

  unset($data['payment_access_key']);
}

/**
 * Function for decode the params.
 *
 * @param array $data
 * @param array $decode_params
 * @param string $key
 *   return encoded string.
 */
function commerce_novalnet_decode_params(&$data, $decode_params, $key) {
  foreach ($decode_params as $param) {
    try {
      $element = base64_decode(strrev($data[$param]));
      $element = pack("H" . strlen($element), $element);
      $element = substr($element, 0, stripos($element, $key));
      $pos = strpos($element, "|");
      if ($pos === FALSE) {
        $result = FALSE;
      }

      $crc = substr($element, 0, $pos);
      $value = trim(substr($element, $pos + 1));
      if ($crc != sprintf('%u', crc32($value))) {
        $result = FALSE;
      }
      if (isset($result) && !$result) {
        drupal_set_message(t('Decode Error'), 'error');
      }
      $data[$param] = $value;
    }
    catch (Exception $e) {
      drupal_set_message(check_plain($e), 'error');
      return FALSE;
    }
  }

}

/**
 * Function for getting hash string.
 *
 * @param array $data
 * @param string $key
 *   return string.
 */
function commerce_novalnet_create_hash($data, $key) {
  $hash_params = array('auth_code', 'product', 'tariff', 'amount', 'test_mode', 'uniqid');
  $hash = '';
  foreach ($hash_params as $params) {
    $hash .= $data[$params];
  }
  return md5($hash . strrev($key));
}

/**
 * Function for adding additional data only for redirect payments.
 *
 * @param array $data
 * @param object $order
 */
function commerce_novalnet_redirect_params(&$data, $order) {
  $data['uniqid'] = commerce_novalnet_get_random_string();
  $return_url                   = url('checkout/' . $order->order_id . '/payment/return/' . $order->data['payment_redirect_key'], array('absolute' => TRUE));
  $data['return_url']           = $return_url;
  $data['return_method']        = 'POST';
  $data['error_return_url']     = $return_url;
  $data['error_return_method']  = 'POST';
  if ($data['key'] != 6) {
    $data['user_variable_0']      = $data['system_url'];
  }
  $data['implementation']       = 'PHP';
}

/**
 * Function for perform order completion process for redirect payments.
 *
 * @param array $response
 * @param object $order
 * @param array $payment_method
 *   return none / bool.
 */
function commerce_novalnet_redirect_process($response, $order, $payment_method) {
  if (in_array($response['status'], array(100, 90))) {
    // Call this function for validate and get the basic data.
    $data = commerce_novalnet_validate_basic_data($payment_method['method_id']);
    commerce_novalnet_check_affiliate_order($response);
    // If the received hash string is not equal to the created hash throw the message.
    if ($response['hash2'] != commerce_novalnet_create_hash($response, $data['payment_access_key'])) {
      drupal_set_message(t('While redirecting some data has been changed. The hash check failed.'), 'error');
      return FALSE;
    }
    $decode_params = array('product', 'tariff', 'test_mode', 'amount', 'auth_code');
    // Call this function for decode the params.
    commerce_novalnet_decode_params($response, $decode_params, $data['payment_access_key']);
    // Call this function for complete the order.
    commerce_novalnet_complete_order($order, $response, $payment_method);
  }
  else {
    drupal_set_message(check_plain($response['status_text']), 'error');
    return FALSE;
  }
}

/**
 * Implements hook_entity_view().
 */
function commerce_novalnet_entity_view($entity, $type, $view_mode, $langcode) {
  if ($type == 'commerce_order' && isset($entity->data['transaction_details'])) {

    $trans_details = '';
    foreach ($entity->data['transaction_details'] as $transaction_details) {
      $trans_details .= '<tr class="views-row-first"><td>' . date('D, m/d/Y - h:i', $transaction_details['update_on']) . '</td><td>' . $transaction_details['message'] . '</td></tr>';
    }

    $entity->content['trans_details']  = array(
      'type'    => 'markup',
      '#markup' => '<table class="nn-trans-details">
                    <tr class="odd">
                      <th colspan=2><b>' . t('Transaction Details') . '<b></th>
                    </tr>
                    <tr>
                      <td><b>' . t('Date') . '<b></td>
                      <td><b>' . t('Description') . '<b></td>
                    </tr> ' . $trans_details . '
                    </table>',
      '#prefix' => '<div class="field field-name-commerce-transaction-details">',
      '#suffix' => '</div>',
    );
  }
}

/**
 * Implements hook_token_info_alter().
 */
function commerce_novalnet_token_info_alter(&$data) {
  $data['tokens']['commerce-order']['novalnet_trxn_details'] = array(
    'name' => t('Novalnet transaction details'),
    'description' => t('Used to display the transaction details'),
  );
  $data['tokens']['commerce-order']['novalnet_barzahlen_slip'] = array(
    'name' => t('Novalnet transaction details'),
    'description' => t('Used to display the Barzahlen slip on order completion page'),
  );
}

/**
 * Implements hook_tokens().
 *
 * @param $type string
 * @param $tokens array
 * @param $data array
 * @param $options array
 *
 * @return array
 */
function commerce_novalnet_tokens($type, $tokens, array $data = array(), $options = array()) {
  $replacements = array();
  if ($type == 'commerce-order' && isset($data['commerce-order']->data['transaction_details'])) {
    if (array_key_exists('novalnet_trxn_details', $tokens)) {
      $trans_details = '';
      foreach ($data['commerce-order']->data['transaction_details'] as $transaction_details) {
        $trans_details .= $transaction_details['message'] . "\n";
      }
      $replacements[$tokens['novalnet_trxn_details']] = $trans_details;
    }
    if (array_key_exists('novalnet_barzahlen_slip', $tokens) && preg_match('/commerce_novalnet_barzahlen/', $data['commerce-order']->data['payment_method'])) {
        $srcipt_url = 'https://cdn.barzahlen.de/js/v2/checkout.js';
        if (!empty($data['commerce-order']->data['test_mode'])) {
            $srcipt_url = 'https://cdn.barzahlen.de/js/v2/checkout-sandbox.js';
        }
        $checkout_token = $data['commerce-order']->data['checkout_token'];
        $replacements[$tokens['novalnet_barzahlen_slip']] = "<script src=" . $srcipt_url . " data-token= " . $checkout_token . " class='bz-checkout'></script><button id='barzahlen_button' class='bz-checkout-btn'>" . t('Pay now with Barzahlen') . "</button>";
    }

  }
  return $replacements;
}

/**
 * Function for get the merchant details from commerce_novalnet_transaction_detail.
 *
 * @param int $order_id
 * @param int $tid
 *
 * @return array
 */
function commerce_novalnet_order_merchant_details($order_id, $tid) {
  $db_result = db_select('commerce_novalnet_transaction_detail', 'vendor_details')
    ->fields('vendor_details', array('vendor_details', 'status', 'payment_key', 'payment_type', 'paid_amount', 'refunded_amount', 'org_total', 'booked', 'payment_details', 'order_id', 'changed'));
  if (!empty($order_id)) {
    $db_result = $db_result->condition('order_id', $order_id);
  }

  $db_result = $db_result->condition('tid', $tid)
    ->execute()
    ->fetchAssoc();

  return $db_result;
}

/**
 * Function for check the order having fraud module and store the transaction details on SESSION.
 *
 * @param array $response
 *
 * @return bool
 */
function commerce_novalnet_fraud_check($response) {
  $fraud_methods = array(
    'pin_by_callback'  => t('You will shortly receive a transaction PIN through phone call to complete the payment.'),
    'pin_by_sms'  => t('You will shortly receive an SMS containing your transaction PIN to complete the payment.'),
  );
  $enabled_fraud = array_flip(array_intersect_key($response, $fraud_methods));

  if (!empty($enabled_fraud)) {
    $data = array(
      'tid'    => $response['tid'],
      'amount' => str_replace(array('.', ','), '', $response['amount']),
      'request' => $response,
      'recurring' => !empty($response['tariff_period']) ? TRUE : FALSE,
    );
    // Call this function to perform session handling.
    commerce_novalnet_session($response['payment_method'], $data);
    drupal_set_message(check_plain($fraud_methods[$enabled_fraud[1]]), 'warning');
    return TRUE;
  }
  return FALSE;
}

/**
 * Function for handle SESSION.
 *
 * @param string $payment
 * @param array $data
 */
function commerce_novalnet_session($payment = '', $data = array()) {
  if (empty($payment)) {
    unset($_SESSION['novalnet']);
  }
  else {
    $_SESSION['novalnet'] = array($payment => $data);
  }
}

/**
 * Function for reset the fraud module pin.
 *
 * @param string $payment_method
 */
function commerce_novalnet_fraud_pin_reset($payment_method) {
  $payment_method = strstr($payment_method, '|', 1);
  if (!isset($_SESSION['novalnet'][$payment_method])) {
    commerce_novalnet_session();
  }
  return array();
}

/**
 * Function for complete the order with fraud module.
 *
 * @param array $pane_values
 * @param array $data
 * @param array $payment_method
 * @param object $order
 */
function commerce_novalnet_fraud_check_complete($pane_values, $data, $payment_method, $order) {
  if (!empty($pane_values['nn_fraud_check'])) {
    $xml_request = array(
      'vendor_id'       => $data['vendor'],
      'vendor_authcode' => $data['auth_code'],
      'tid'             => $_SESSION['novalnet'][$data['payment_method']]['tid'],
      'lang'            => $data['lang'],
    );
    if (!empty($pane_values['nn_fraud_resend'])) {
      $xml_request['request_type'] = 'TRANSMIT_PIN_AGAIN';
    }
    else {
      $xml_request['request_type']  = 'PIN_STATUS';
      $xml_request['pin'] = trim($pane_values['nn_fraud_pin']);
    }
    // Call this function for send request to Novalnet server.
    $response = commerce_novalnet_drupal_request($xml_request, COMMERCE_NOVALNET_XML_URL, TRUE);

    $result = (array) simplexml_load_string($response);

    // If the transaction was succeeded.
    if (isset($result['status']) && $result['status'] == 100) {
      $response = array_merge($_SESSION['novalnet'][$data['payment_method']]['request'], $result);
      commerce_novalnet_complete_order($order, $response, $payment_method);
    }
    // If the transaction was failure.
    else {
      if (isset($result['status'])) {
        if (in_array($result['status'], array('0529008', '0529006', '0529010'))) {
          unset($_SESSION['novalnet'][$data['payment_method']]);
        }
        if ($result['status'] == '0529006') {
          $_SESSION['novalnet_error_payment'] = array($data['payment_method'] => array('error_time' => time()));
        }
        drupal_set_message($result['status_message'], 'warning');
      }
      else {
        $error_message = (array) $result['pin_status']->status_message;
        drupal_set_message($error_message[0], 'warning');
      }
      drupal_goto('checkout/' . $order->order_id . '/review');
    }
  }
}

/**
 * Function for add recurring params.
 *
 * @param array $data
 * @param array $global_setting
 *
 * @return bool
 */
function commerce_novalnet_add_recurring_data(&$data, $global_setting) {
  if (isset($global_setting['subs_product'])) {
    $subs_product = $global_setting['subs_product'];
    if (count($subs_product) > 1) {
      drupal_set_message(t('More than one recurring product not allowed'), 'error');
      drupal_goto('cart/checkout');
      return FALSE;
    }
    $product = commerce_product_load($subs_product[0]['product_id']);

    $data['tariff_period']         = isset($product->commerce_recurring_ini_period['und'][0]) ? $product->commerce_recurring_ini_period['und'][0] : ( isset($product->commerce_recurring_rec_period['und'][0]) ? $product->commerce_recurring_rec_period['und'][0] : '' );
    $data['tariff_period']         = commerce_novalnet_generate_interval($data['tariff_period']);

    $data['tariff_period2']        = isset($product->commerce_recurring_rec_period['und'][0]) ? $product->commerce_recurring_rec_period['und'][0] : ( isset($product->commerce_recurring_rec_period['und'][0]) ? $product->commerce_recurring_rec_period['und'][0] : '') ;
    $data['tariff_period2']        = commerce_novalnet_generate_interval($data['tariff_period2']);

    $data['tariff_period2_amount'] = ( isset($product->commerce_recurring_rec_price['und'][0]['amount']) ? $product->commerce_recurring_rec_price['und'][0]['amount'] : $product->commerce_price['und'][0]['amount'] ) * $subs_product[0]['quantity'];

  }
  return TRUE;
}

/**
 * Function for generate interval as per novalnet tariff period format.
 *
 * @param array $interval
 *
 * @return string
 */
function commerce_novalnet_generate_interval($interval) {
  if ($interval['period'] == 'week') {
    $interval = array(
      'period'   => 'day',
      'interval' => $interval['interval'] * 7,
    );
  }
  return str_replace(array('day', 'month', 'year'), array('d', 'm', 'y'), $interval['interval'] . $interval['period']);
}

/**
 * Function for check guarantee payment.
 *
 * @param array $payment_settings
 * @param object $order
 *
 * @return string / bool
 */
function commerce_novalnet_check_guarantee_payment($payment_settings, $order) {
  if (empty($payment_settings) || empty($order)) {
    return TRUE;
  }

  $order_total    = $order->commerce_order_total['und'][0]['amount'];
  $order_currency = $order->commerce_order_total['und'][0]['currency_code'];

  $billing_wrapper   = entity_metadata_wrapper('commerce_order', $order);
  $billing_details  = $billing_wrapper->commerce_customer_billing->commerce_customer_address->value();

  $shipping_details = array();
  if (!empty($order->commerce_customer_shipping)) {
    $shipping_details = $billing_wrapper->commerce_customer_shipping->commerce_customer_address->value();
  }

  $billing_shipping_diff = array_diff($shipping_details, $billing_details);
  $valid_billing_address = (empty($shipping_details) || empty($billing_shipping_diff));
  $min_amount = !empty($payment_settings['guarantee_payment']['min_amount']) ? $payment_settings['guarantee_payment']['min_amount'] : 2000; 
  if (!empty($payment_settings['guarantee_payment']['enable_guarantee']) && $order_total >= $min_amount && in_array($billing_details['country'], array('DE', 'AT', 'CH')) && $order_currency == 'EUR' && $valid_billing_address) {
    return 'guarantee_payment';
  }
  elseif (empty($payment_settings['guarantee_payment']['enable_guarantee']) || !empty($payment_settings['guarantee_payment']['force_guarantee'])) {
    return 'normal_payment';
  }
  else {
    return FALSE;
  }
}

/**
 * Function for check the age limit for guarantee payments.
 *
 * @param array $data
 * @param string $birth_date
 * @param int $force_guarantee
 */
function commerce_novalnet_check_age_limit(&$data, $birth_date, $force_guarantee) {
  $bdate = new DateTime($birth_date);
  $today = new DateTime('today');

  if ($bdate->diff($today)->y > 17) {
    $data['key'] = constant(strtoupper($data['payment_method']) . '_GUARANTEE_KEY');
    $data['payment_type'] = constant(strtoupper($data['payment_method']) . '_GUARANTEE_TYPE');
    $data['birth_date'] = $birth_date;
    return TRUE;
  }

  if (empty($force_guarantee)) {
    drupal_get_messages();
    drupal_set_message(t('You need to be at least 18 years old'), 'error');
    return FALSE;
  }

  return TRUE;
}

/**
 * Function for check zero amount transaction.
 *
 * @param array $data
 * @param array $payment_method
 */
function commerce_novalnet_zero_amount_check(&$data, $shopping_type) {
  if ($shopping_type == 'zero_amount' && !isset($data['tariff_period'])) {
    $data['amount'] = 0;
  }
}

/**
 * Function for build the payment name with logo.
 *
 * @param string $title
 * @param array $payment_method
 * @param array $settings
 *   return string.
 */
function commerce_novalnet_payment_display_title($title, $payment_method, $settings = array()) {

  $variables = array(
    'path' => NOVALNET_IMAGES_PATH . $payment_method . '.png',
    'alt' => $title,
    'title' => $title,
  );
  $payment_image = l(theme('image', $variables), t('http://www.novalnet.com'), array('html' => TRUE, 'attributes' => array('target' => '_blank')));

  if ($payment_method == 'commerce_novalnet_cc') {
    foreach (array('commerce_novalnet_cc_amex', 'commerce_novalnet_cc_maestro', 'commerce_novalnet_cc_cartasi') as $logo) {
      if (!empty($settings[$logo])) {
        $variables = array(
          'path' => NOVALNET_IMAGES_PATH . $logo . '.png',
          'alt' => $title,
          'title' => $title,
        );
        $payment_image .= ' ' . l(theme('image', $variables), t('http://www.novalnet.com'), array('html' => TRUE, 'attributes' => array('target' => '_blank')));
      }
    }
  }
  return $title . ' ' . $payment_image;
}

/**
 * Implements hook_mail().
 *
 * @param string $key
 * @param array $message
 * @param array $params
 */
function commerce_novalnet_mail($key, &$message, $params) {
  $message['subject'] = $params['subject'];
  $message['body'][]  = (isset($params['headers'][0]) && $params['headers'][0] == 'Content-Type: text/plain; charset=utf-8') ? $params['body'] : check_markup($params['body']);
  $message['headers']['MIME-Version'] = '1.0';
  $message['headers']['Content-Type'] = (isset($params['headers'][0]) && $params['headers'][0] == 'Content-Type: text/plain; charset=utf-8')  ? 'text/plain; charset=utf-8' : 'text/html; charset=UTF-8; format=flowed'; 
  $message['headers']['Content-Transfer-Encoding'] = '8Bit';
  $message['headers']['X-Mailer'] = 'Drupal';
  $message['headers'] += $params['headers'];
}

/**
 * Function for add test transaction message in payment page.
 *
 * @param int $settings
 *
 * @return string
 */
function commerce_novalnet_test_payment_info($settings) {
  $payment_desc  = !empty($settings['enduser_info']) ? '<div>' . $settings['enduser_info'] . '</div>' : '';
  $payment_desc .= !empty($settings['test_mode']) ? '<div class="nn_error">' . t('The payment will be processed in the test mode therefore amount for this transaction will not be charged') . '</div>' : '';

  return $payment_desc;
}

/**
 * Function for suspended subscription on cron runs.
 */
function commerce_novalnet_cancel_recurring_on_cron() {
  if (!db_table_exists('commerce_recurring')) {
    return TRUE;
  }

  $cancel_recurrings = db_select('commerce_recurring', 'commerce_recurring')
    ->fields('commerce_recurring', array('id'))
    ->condition('status', 0)
    ->execute()
    ->fetchAll();
  foreach ($cancel_recurrings as $recurring) {
    $recurring_entity = commerce_recurring_load($recurring->id);
    $order = commerce_order_load($recurring_entity->commerce_recurring_order['und'][0]['target_id']);
    if (empty($order->data['subs']) || (isset($order->data['subs']['status']) && $order->data['subs']['status'] == 'suspended') || !empty($order->data['subs']['cancel'])) {
      continue;
    }

    $order_details = commerce_novalnet_order_merchant_details($order->order_id, $order->data['tid']);
    $vendor_details = unserialize($order_details['vendor_details']);

    $data = array(
      'vendor_id'       => $vendor_details['vendor'],
      'vendor_authcode' => $vendor_details['auth_code'],
      'product_id'      => $vendor_details['product'],
      'request_type'    => 'SUBSCRIPTION_PAUSE',
      'tid'             => $order->data['tid'],
      'subs_id'         => $order->data['subs']['subs_id'],
      'suspend'         => 1,
    );
    $response = commerce_novalnet_drupal_request($data, COMMERCE_NOVALNET_XML_URL, TRUE);
    $response = (array) simplexml_load_string($response);
    if (isset($response['status']) && $response['status'] == 100) {
      commerce_novalnet_add_order_history(array('order_id' => $order->order_id, 'message' => t('This subscription transaction has been suspended on @date', array('@date' => date('Y-m-d h:i:s')))));
    }
    $order->data['subs']['status'] = 'suspended';
    commerce_order_save($order);
  }
}

/**
 * Function to get status text.
 *
 * @param $request
 *
 * @return message
 */
function commerce_novalnet_get_status_text($request) {
  if (!empty($request['status_message'])) {
    return $request['status_message'];
  }
  elseif (!empty($request['status_desc'])) {
    return $request['status_desc'];
  }
  elseif (!empty($request['status_text'])) {
    return $request['status_text'];
  }
  else {
    return t('Payment was not successful. An error occurred.');
  }
}

/**
* Get Server / Remote IP address
* @param $type
* 
* @return $ip_address
*/
function commerce_novalnet_get_req_ip_address($type = 'REMOTE_ADDR') {
	// Check to determine the IP address type
    if ($type == 'SERVER_ADDR') {
        if (empty($_SERVER['SERVER_ADDR'])) {
            // Handled for IIS server
            $ip_address = gethostbyname($_SERVER['SERVER_NAME']);
        } else {
            $ip_address = $_SERVER['SERVER_ADDR'];
        }
    } else { // For remote address
        $ip_address = ip_address();
    }   
   
    return $ip_address;
	
}

/**
 * Function for order confirmation mail.
 *
 * @param int $order
 * @param string $message
 * 
 */
function commerce_novalnet_transaction_confirmation_mail($message, $order) {	
  global $language;

  $billing_wrapper  = entity_metadata_wrapper('commerce_order', $order);
  $billing_details  = $billing_wrapper->commerce_customer_billing->commerce_customer_address->value();
  $content = '<body style="background:#F6F6F6; font-family:Verdana, Arial, Helvetica, sans-serif; font-size:14px; margin:0; padding:0;">
					<div style="width:55%;height:auto;margin: 0 auto;background:rgb(247, 247, 247);border: 2px solid rgb(223, 216, 216);border-radius: 5px;box-shadow: 1px 7px 10px -2px #ccc;">
						<div style="min-height: 300px;padding:20px;">
							<table cellspacing="0" cellpadding="0" border="0" width="100%">

								<tr><b>Dear Mr./Ms./Mrs.</b>'.$billing_details['first_name'] .' '. $billing_details['last_name'].'</tr></br></br>

								<tr>'.t('We are pleased to inform you that your order has been confirmed.').'</tr></br></br>

								<tr><b>Payment Information:</b></br> </br>'								
									.$message.		                
								'</tr></br>                                            

							</table>
						</div>
						<div style="width:100%;height:20px;background:#00669D;"></div>
					</div>
				</body>';				
  $params = array(
    'body'    => $content,
    'subject' => t('Order Confirmation - Your Order @order_no with @shop_name has been confirmed!', array('@order_no' => $order->order_number, '@shop_name' => variable_get('site_name'))),
    'headers' => array(),
  );
  drupal_mail('commerce_novalnet', '', $order->mail, $language->language, $params, variable_get('site_mail'), TRUE);   
}
