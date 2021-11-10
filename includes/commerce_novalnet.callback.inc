<?php

 /**
  * @file
  * Novalnet payment method module
  * This module is used for real time processing of
  * Novalnet transaction of customers.
  *
  * Copyright (c) Novalnet AG.
  *
  * Released under the GNU General Public License
  * This free contribution made by request.
  * If you have found this script useful a small
  * recommendation as well as a comment on merchant form
  * would be greatly appreciated.
  *
  *  Script : commerce_novalnet.callback.inc
  */

/* commerce_novalnet_PAYMENT defined with novalnet PAYMENT_TYPE */
define('NOVALNET_PAYMENTS', serialize(array(
  'commerce_novalnet_cc' => array('CREDITCARD', 'CREDITCARD_BOOKBACK', 'CREDITCARD_CHARGEBACK', 'CREDIT_ENTRY_CREDITCARD', 'SUBSCRIPTION_STOP', 'DEBT_COLLECTION_CREDITCARD'),
  'commerce_novalnet_sepa' => array('DIRECT_DEBIT_SEPA', 'RETURN_DEBIT_SEPA', 'SUBSCRIPTION_STOP', 'DEBT_COLLECTION_SEPA', 'CREDIT_ENTRY_SEPA', 'GUARANTEED_DIRECT_DEBIT_SEPA', 'REFUND_BY_BANK_TRANSFER_EU'),
  'commerce_novalnet_ideal' => array('IDEAL', 'REFUND_BY_BANK_TRANSFER_EU', 'REVERSAL', 'ONLINE_TRANSFER_CREDIT'),
  'commerce_novalnet_instantbank' => array('ONLINE_TRANSFER', 'ONLINE_TRANSFER_CREDIT', 'REFUND_BY_BANK_TRANSFER_EU', 'REVERSAL'),
  'commerce_novalnet_eps' => array('EPS', 'REFUND_BY_BANK_TRANSFER_EU'),
  'commerce_novalnet_paypal' => array('PAYPAL', 'PAYPAL_BOOKBACK', 'SUBSCRIPTION_STOP'),
  'commerce_novalnet_giropay' => array('GIROPAY', 'REFUND_BY_BANK_TRANSFER_EU'),
  'commerce_novalnet_prepayment' => array('INVOICE_START', 'INVOICE_CREDIT', 'SUBSCRIPTION_STOP'),
  'commerce_novalnet_invoice' => array('INVOICE_START', 'INVOICE_CREDIT', 'SUBSCRIPTION_STOP', 'GUARANTEED_INVOICE_START', 'GUARANTEED_INVOICE_CREDIT'),
  'commerce_novalnet_przelewy24' => array('PRZELEWY24', 'PRZELEWY24_REFUND'),
)));

define('COMMUNICATION_FAILURE_PAYMENTS', serialize(array('IDEAL', 'ONLINE_TRANSFER', 'EPS', 'PAYPAL', 'CREDITCARD', 'PRZELEWY24', 'GIROPAY')));

define('NORMAL_PAYMENTS', serialize(array('CREDITCARD', 'DIRECT_DEBIT_SEPA', 'IDEAL', 'ONLINE_TRANSFER', 'PAYPAL', 'EPS', 'GIROPAY', 'INVOICE_START', 'GUARANTEED_INVOICE_START', 'GUARANTEED_DIRECT_DEBIT_SEPA', 'PRZELEWY24')));
define('NOVALNET_SUCCESS_STATUS', serialize(array(85, 86, 90, 91, 98, 99, 100)));
define('COLLECTIONS', serialize(array('INVOICE_CREDIT', 'GUARANTEED_INVOICE_CREDIT', 'CREDIT_ENTRY_CREDITCARD', 'CREDIT_ENTRY_SEPA', 'DEBT_COLLECTION_SEPA', 'DEBT_COLLECTION_CREDITCARD', 'GUARANTEED_DEBT_COLLECTION_SEPA', 'ONLINE_TRANSFER_CREDIT')));
define('CHARGEBACK', serialize(array('RETURN_DEBIT_SEPA', 'CREDITCARD_BOOKBACK', 'CREDITCARD_CHARGEBACK', 'REFUND_BY_BANK_TRANSFER_EU', 'PAYPAL_BOOKBACK', 'PRZELEWY24_REFUND')));
define('NOVALNET_IP', serialize(array('195.143.189.210', '195.143.189.214')));
define('LINEBREAK', empty($_SERVER['HTTP_HOST']) ? PHP_EOL : '<br />');

/**
 * Menu call back for the menu commerce_novalnet_callback.
 */
function commerce_novalnet_callback() {
  $callback_settings = variable_get('novalnet_callback_settings', '');
  $request = array_map('trim', array_map('check_plain', $_REQUEST));
  // Call this function for check the IP.
  if (commerce_novalnet_check_request_ip($callback_settings, $request)) {
    // Call this function for check the required params received from novalnet server.
    commerce_novalnet_check_required_params($callback_settings, $request);
    // Call this function for perform call-back actions.
    commerce_novalnet_callback_process($callback_settings, $request);
  }
}

/**
 * Function for perform call-back actions.
 *
 * @params array $callback_settings
 * @params array $request
 */
function commerce_novalnet_callback_process($callback_settings, $request) {

  $order_id            = !empty($request['order_no']) ? $request['order_no'] : '';

  // Call this function to get the merchant details from the DB.
  $order_details       = commerce_novalnet_order_merchant_details($order_id, $request['org_tid']);

  $request['order_no'] = !empty($order_id) ? $order_id : (!empty($order_details['order_id']) ? $order_details['order_id'] : '');
  $order = empty($request['order_no']) ? '' : commerce_order_load($request['order_no']);

  if (empty($order_details) && empty($order)) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Order Mapping failed'));
  }
  if (!empty($order)) {
    $order_payment = strstr($order->data['payment_method'], '|', 1);
    $payment_mismatched = TRUE;
    foreach (unserialize(NOVALNET_PAYMENTS) as $payment_method => $payments) {
      if (in_array($request['payment_type'], $payments) && $payment_method == $order_payment) {
        $payment_mismatched = FALSE;
        break;
      }
    }
    if ($payment_mismatched) {
      // Call this function for show the error message.
      commerce_novalnet_show_callback_message($callback_settings, t('Payment Type is mismatched'));
    }    $request['payment_method'] = $payment_method;

    $payment_method      = commerce_payment_method_instance_load($order->data['payment_method']);
    $callback_settings['global_setting'] = variable_get('novalnet_global_settings', '');

    if (empty($order_details) && in_array($request['payment_type'], unserialize(COMMUNICATION_FAILURE_PAYMENTS))) {
      commerce_novalnet_communication_failure($callback_settings, $request, $order);
    }

    switch ($request['callback_type']) {
      case 'zerolevel':
        if ($request['payment_type'] == 'PAYPAL') {
          commerce_novalnet_check_status($request, $callback_settings);
        }
        commerce_novalnet_process_zerolevel($callback_settings, $request, $order_details, $order, $payment_method);
        break;

      case 'chargeback':
        commerce_novalnet_check_status($request, $callback_settings);
        commerce_novalnet_process_chargeback($callback_settings, $request, $order);
        break;

      case 'credit':
        commerce_novalnet_check_status($request, $callback_settings);
        commerce_novalnet_process_credit($callback_settings, $request, $order_details, $order, $payment_method);
        break;

      case 'recurring':
        commerce_novalnet_process_recurring($callback_settings, $request, $order_details, $order);
        break;
    }
  }
}

/**
 * Function for checks the status.
 *
 * @param array $request
 * @param array $callback_settings
 */
function commerce_novalnet_check_status($request, $callback_settings) {
  if ($request['status'] != 100 || $request['tid_status'] != 100) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Invalid status/tid_status : Only 100 is allowed'));
  }
}

/**
 * Function for validate the server IP.
 *
 * @param array $callback_settings
 * @param array $request
 *
 * @return bool
 */
function commerce_novalnet_check_request_ip($callback_settings, $request) {
  $ip_address = ip_address();
  if (empty($callback_settings['callback_test_mode']) && !in_array($ip_address, unserialize(NOVALNET_IP))) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Unauthorized access from the IP [@ip]', array('@ip' => $ip_address)), TRUE);
  }
  if (empty($request)) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('No params passed over!'));
  }  return TRUE;
}

/**
 * Function for display the messages.
 *
 * @param array $callback_settings
 * @param string $message
 * @param bool $ip_error
 */
function commerce_novalnet_show_callback_message($callback_settings, $message, $ip_error = FALSE) {
  if (!empty($callback_settings['callback_debug_mode']) || $ip_error) {
    echo html_entity_decode($message);
  }
  exit;
}

/**
 * Function for validate required params.
 *
 * @param array $callback_settings
 * @param array $request
 *   return bool.
 */
function commerce_novalnet_check_required_params($callback_settings, &$request) {
  $required_params  = array('vendor_id', 'tid', 'payment_type', 'status', 'amount', 'tid_status');
  $payment_type = isset($request['payment_type']) ? $request['payment_type'] : '';

  if (!empty($request['subs_billing']) || $payment_type == 'SUBSCRIPTION_STOP') {
    $request['callback_type'] = 'recurring';
    $required_params[] = 'signup_tid';
    if ($payment_type == 'SUBSCRIPTION_STOP') {
      unset($required_params[4]);
    }
  }
  elseif (in_array($payment_type, unserialize(CHARGEBACK))) {
    $request['callback_type'] = 'chargeback';
    $required_params[]       = 'tid_payment';
  }
  elseif (!empty($request['vendor_activation'])) {
    $request['callback_type'] = 'affiliate';
    $required_params         = array('vendor_id', 'vendor_authcode', 'product_id', 'product_url', 'activation_date', 'aff_id', 'aff_authcode', 'aff_accesskey');
  }
  elseif (in_array($payment_type, unserialize(COLLECTIONS))) {
    $request['callback_type'] = 'credit';
    $required_params[]       = 'tid_payment';
  }
  elseif (in_array($payment_type, unserialize(NORMAL_PAYMENTS))) {
    $request['callback_type'] = 'zerolevel';
  }

  foreach ($required_params as $params) {
    if (empty($request[$params])) {
      $missing_params[] = $params;
    }
  }

  if (isset($missing_params)) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Missing params { @params }', array('@params' => implode(', ', $missing_params))));
  }
  if ($request['callback_type'] == 'affiliate') {
    // Call this function for store the affiliate information.
    commerce_novalnet_process_affiliate($callback_settings, $request);
  }
  $novalnet_payments = unserialize(NOVALNET_PAYMENTS);
  $novalnet_payments = array_reduce($novalnet_payments, 'array_merge', array());
  if (!in_array($payment_type, $novalnet_payments)) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Invalid payment type { @payment_type }', array('@payment_type' => $payment_type)));
  }
  foreach (array('tid', 'tid_payment', 'signup_tid') as $tid) {
    if (in_array($tid, $required_params)) {
      if (!preg_match('/^\d{17}$/', $request[$tid])) {
        // Call this function for show the error message.
        commerce_novalnet_show_callback_message($callback_settings, t('Invalid TID { @tid }', array('@tid' => $tid)));
      }      $request['org_tid'] = $request[$tid];
    }
  }
  if (!isset($request['callback_type'])) {
    $request['callback_type'] = 'credit';
  }
  return TRUE;
}

/**
 * Function for perform charge back action.
 *
 * @param array $callback_settings
 * @param array $request
 * @param object $order
 */
function commerce_novalnet_process_chargeback($callback_settings, $request, $order) {

  $tarray = array(
    '@otid' => $request['org_tid'],
    '@amt'  => commerce_currency_format($request['amount'], $request['currency']),
    '@date' => date('d.m.Y H:i:s'),
    '@tid'  => $request['tid'],
  );

  $callback_settings['mail_body'] = t('Novalnet callback received. Chargeback executed successfully for the TID : @otid amount: @amt on @date. The subsequent TID: @tid', $tarray);

  if (in_array($request['payment_type'], array('CREDITCARD_BOOKBACK', 'PAYPAL_BOOKBACK', 'PRZELEWY24_REFUND'))) {
    $callback_settings['mail_body'] = t('Novalnet callback received. Refund/Bookback executed successfully for the TID : @otid amount: @amt on @date. The subsequent TID: @tid', $tarray);
  }

  $message = array(
    'order_id' => $order->order_id,
    'message'  => $callback_settings['mail_body'],
  );
  commerce_novalnet_add_order_history($message);
  // Call this function for send a mail.
  commerce_novalnet_send_mail($callback_settings, $order);
}

/**
 * Function for perform subscription stop and subscription renewal.
 *
 * @param array $callback_settings
 * @param array $request
 * @param array $order_details
 * @param object $order
 */
function commerce_novalnet_process_recurring($callback_settings, $request, $order_details, $order) {
  if ($request['status'] != 100) {
    $request['payment_type']       = 'SUBSCRIPTION_STOP';
    $request['termination_reason'] = (!empty($request['status_message'])) ? $request['status_message'] : commerce_novalnet_get_status_text($request);
  }

  $recurring_entity = commerce_recurring_load_by_order($order);

  if (empty($recurring_entity)) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Subscription is not active.'));
  }  if ($request['payment_type'] == 'SUBSCRIPTION_STOP') {
    $termination_reason = !empty($request['termination_reason']) ? $request['termination_reason'] : commerce_novalnet_get_status_text($request);
    commerce_recurring_stop_recurring($recurring_entity[0]);
    $order->data['subs']['cancel'] = TRUE;
    commerce_order_save($order);
    $message = array(
      'order_id' => $order->order_id,
      'message'  => t('Novalnet callback script received. Subscription has been stopped for the TID: @tid on @date @time. Subscription has been canceled due to: @reason', array('@reason' => $termination_reason, '@tid' => $request['org_tid'], '@date' => date('d.m.Y'), '@time' => date('H:i:s'))),
    );

    $callback_settings['mail_body'] = $message['message'];
    commerce_novalnet_add_order_history($message, FALSE);

    $global_setting = $callback_settings['global_setting'];

    foreach ($recurring_entity[0]->commerce_recurring_order['und'] as $orders) {
      // Core function for update the order status.
      commerce_order_status_update(commerce_order_load($orders['target_id']), $global_setting['subs']['cancel_status'], FALSE, TRUE, t('Order state updated'));
    }

    // Call this function for send a mail.
    commerce_novalnet_send_mail($callback_settings, $order);
  }

  module_load_include('inc', 'commerce_recurring', 'commerce_recurring.rules');

  $request['paid_until'] = !empty($request['next_subs_cycle']) ? $request['next_subs_cycle'] : $request['paid_until'];

  $recurring_order = commerce_recurring_rules_generate_order_from_recurring($recurring_entity[0], strtotime($request['paid_until']), $order->commerce_customer_billing, $order->commerce_customer_shipping);
  if (empty($recurring_order)) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Subscription is not active.'));
  }  if (empty($recurring_order['commerce_order']->commerce_customer_billing)) {
    $recurring_order['commerce_order']->commerce_customer_billing = $order->commerce_customer_billing;
  }

  $vendor_details = unserialize($order_details['vendor_details']);
  $request = array_merge($request, $vendor_details);
  $request['key'] = $order_details['payment_key'];
  $request['customer_no'] = $recurring_order['commerce_order']->uid;
  $request['order_no'] = $recurring_order['commerce_order']->order_id;
  $request['invoice_ref'] = 'BNR-' . $request['product'] . '-' . $recurring_order['commerce_order']->order_id;
  $payment_method = commerce_payment_method_instance_load($order->data['payment_method']);
  $recurring_order['commerce_order']->data['payment_method'] = $order->data['payment_method'];

  global $base_url;

  $new_order = commerce_novalnet_complete_order($recurring_order['commerce_order'], $request, $payment_method, $order->order_id);

  $message = t("Novalnet Callback Script executed successfully for the subscription TID: @tid with amount @amount on @date. Please refer PAID transaction in our Novalnet Merchant Administration with the TID: @ttid. <br>Next charging date : @ndate <br>Reference Order : @order_no", array('@tid' => $request['org_tid'], '@ttid' => $request['tid'], '@amount' => commerce_currency_format($request['amount'], $request['currency']), '@date' => date('d.m.Y H:i:s'), '@order_no' => $order->order_id, '@ndate' => $request['paid_until']));

  $callback_settings['mail_body'] = $message;

  // Call this function for send a mail.
  commerce_novalnet_send_mail($callback_settings, $order);
}

/**
 * Function for perform amount credit.
 *
 * @param array $callback_settings
 * @param array $request
 * @param array $order_details
 * @param object $order
 * @param array $payment_method
 */
function commerce_novalnet_process_zerolevel($callback_settings, $request, $order_details, $order, $payment_method) {
  $paypal_completed   = (in_array($request['tid_status'], array(85, 90) && $request['payment_type'] == 'PAYPAL'));
  $przelewy_completed = ($request['tid_status'] == 86 && $request['payment_type'] == 'PRZELEWY24');

  if ($order_details['org_total'] <= $order_details['paid_amount'] || $paypal_completed || $przelewy_completed || !in_array($request['payment_type'], array('PAYPAL', 'PRZELEWY24'))) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Payment type (@ptype) is not applicable for this process!', array('@ptype' => $request['payment_type'])));
  }
  $request['amount']   = $order->commerce_order_total['und'][0]['amount'];
  $request['currency'] = $order->commerce_order_total['und'][0]['currency_code'];
  $request['status']   = $payment_method['settings']['order_status'];

  $global_setting = $callback_settings['global_setting'];

  $tarray = array(
    '@tid'  => $request['org_tid'],
    '@amt'  => commerce_currency_format($request['amount'], $request['currency']),
    '@date' => date('d.m.Y H:i:s'),
  );
  $callback_settings['mail_body'] = t('Novalnet Callback Script executed successfully for the TID: @tid with amount @amt on @date', $tarray);

  if ($request['tid_status'] != 100) {
    $callback_settings['mail_body'] = t('The transaction has been canceled due to: @reason', array('@reason' => commerce_novalnet_get_status_text($request)));
    $request['amount']         = 0;
    $request['failure_status'] = 1;
    $request['status']         = $global_setting['onhold_void_cancel_status'];
  }

  commerce_novalnet_add_credit_transaction($request, $callback_settings, $payment_method, $order, $request['amount']);
  // Core function for update the order status.
  commerce_order_status_update($order, $request['status'], FALSE, TRUE, t('Order state updated'));
  // Call this function for send a mail.
  commerce_novalnet_send_mail($callback_settings, $order);
}

/**
 * Function for perform amount credit.
 *
 * @param array $callback_settings
 * @param array $request
 * @param array $order_details
 * @param object $order
 * @param array $payment_method
 */
function commerce_novalnet_process_credit($callback_settings, $request, $order_details, $order, $payment_method) {
  if (!in_array($order_details['payment_type'], array('commerce_novalnet_invoice', 'commerce_novalnet_prepayment', 'commerce_novalnet_ideal', 'commerce_novalnet_instantbank'))) {
    // Call this function for show the error message.
    commerce_novalnet_show_callback_message($callback_settings, t('Payment type (@ptype) is not applicable for this process!', array('@ptype' => $request['payment_type'])));
  }  if ($order_details['org_total'] <= $order_details['paid_amount']) {
    // Call this function for show the message.
    commerce_novalnet_show_callback_message($callback_settings, t('Order got completed already! Refer order : @oid', array('@oid' => $order->order_id)));
  }  $request['status']   = $payment_method['settings']['callback_status'];
  $request['currency'] = $order->commerce_order_total['und'][0]['currency_code'];
  if ($request['payment_type'] == 'PAYPAL') {
    $request['amount'] = $order->commerce_order_total['und'][0]['amount'];
    $request['status']   = $payment_method['settings']['paypal_pending_status'];
    $tarray = array(
      '@tid'  => $request['org_tid'],
      '@amt'  => commerce_currency_format($request['amount'], $request['currency']),
      '@date' => date('d.m.Y H:i:s'),
    );
    $callback_settings['mail_body'] = t('Novalnet Callback Script executed successfully for the TID: @tid with amount @amt on @date', $tarray);
  }
  else {
    $tarray = array(
      '@tid'  => $request['org_tid'],
      '@amt'  => commerce_currency_format($request['amount'], $request['currency']),
      '@date' => date('d.m.Y H:i:s'),
      '@ttid' => $request['tid'],
    );
    $callback_settings['mail_body'] = t('Novalnet Callback Script executed successfully for the TID: @tid with amount @amt on @date. Please refer PAID transaction in our Novalnet Merchant Administration with the TID: @ttid', $tarray);
  }
  $paid_amount = $order_details['paid_amount'] + $request['amount'];
  commerce_novalnet_add_credit_transaction($request, $callback_settings, $payment_method, $order, $paid_amount);

  if ($paid_amount > $order_details['org_total']) {
    $callback_settings['greater_notify'] .= t('Paid amount is greater than the order amount.');
  }

  if ($paid_amount >= $order_details['org_total']) {
    foreach ($order->data['transaction_details'] as $key => $transaction_details) {
      $order->data['transaction_details'][$key]['message'] = preg_replace('|<div id="prepayment_comments">(.+?)</div>|s', '', $transaction_details['message']);
    }
    commerce_order_save($order);
    if ($request['payment_type'] != 'ONLINE_TRANSFER_CREDIT') {
      // Core function for update the order status.
      commerce_order_status_update($order, $request['status'], FALSE, TRUE, t('Order state updated'));
    }
    else {
      $callback_settings['mail_body'] .= t('Please update order status');
    }
  }
  // Call this function for send a mail.
  commerce_novalnet_send_mail($callback_settings, $order);
}

/**
 * Function for perform communication failure.
 *
 * @param array $callback_settings
 * @param array $request
 * @param object $order
 */
function commerce_novalnet_communication_failure($callback_settings, $request, $order) {

  $global_setting = $callback_settings['global_setting'];
  $payment_method = commerce_payment_method_instance_load($order->data['payment_method']);

  if (in_array($request['status'], array(90, 100)) && in_array($request['tid_status'], unserialize(NOVALNET_SUCCESS_STATUS))) {
    $request['vendor_authcode'] = $global_setting['auth_code'];
    $request['customer_no']     = $order->uid;
    $request['key']             = constant(strtoupper(strstr($order->data['payment_method'], '|', 1)) . '_KEY');

    $request['vendor']          = !empty($request['vendor_id']) ? $request['vendor_id'] : $request['vendor'];
    $request['auth_code']       = !empty($request['vendor_authcode']) ? $request['vendor_authcode'] : $request['auth_code'];
    $request['product']         = !empty($request['product_id']) ? $request['product_id'] : $request['product'];
    $request['tariff']          = !empty($request['tariff_id']) ? $request['tariff_id'] : $request['tariff'];

    // Call this function for complete  the order.
    $order_data = commerce_novalnet_complete_order($order, $request, $payment_method, 'communication_failure');
    $callback_settings['mail_body']  = $order_data['trans_message'];
    // Call this function for send a mail.
    commerce_novalnet_send_mail($callback_settings, $order);
  }
  else {

    $test_order = !empty($request['test_mode']) ? t('Test order') : '';
    $trans_message = $test_order . '<br>' . commerce_novalnet_get_status_text($request) . '<br>' . t('Novalnet Transaction ID : @tid', array('@tid' => $request['tid']));
    $order->data['tid'] = $request['tid'];
    $order->data['transaction_details'][] = array('update_on' => time(), 'message' => $trans_message);
    $request['amount']      = 0;
    $request['org_amount']  = $order->commerce_order_total['und']['0']['amount'];
    commerce_novalnet_store_transaction($request, $payment_method);
    commerce_order_save($order);
    // Core function for update the order status.
    commerce_order_status_update($order, $global_setting['onhold_void_cancel_status'], FALSE, TRUE, t('Order state updated'));
    commerce_novalnet_show_callback_message($callback_settings, $trans_message);
  }
}

/**
 * Function for store the affiliate details.
 *
 * @param array $callback_settings
 * @param array $request
 */
function commerce_novalnet_process_affiliate($callback_settings, $request) {
  $aff_data = array(
    'vendor_id'        => $request['vendor_id'],
    'vendor_authcode'  => $request['vendor_authcode'],
    'product_id'       => $request['product_id'],
    'product_url'      => $request['product_url'],
    'activation_date'  => !empty($request['activation_date']) ? $request['activation_date'] : date('d-m-y H:i:s'),
    'aff_id'           => $request['aff_id'],
    'aff_authcode'     => $request['aff_authcode'],
    'aff_accesskey'    => $request['aff_accesskey'],
  );
  db_insert('commerce_novalnet_affiliate_detail')->fields($aff_data)->execute();
  $callback_settings['mail_body'] = t('Novalnet callback script executed successfully with Novalnet account activation information.');
  commerce_novalnet_send_mail($callback_settings);
}

/**
 * Function to send mail on successful execution.
 *
 * @param array $callback_settings
 */
function commerce_novalnet_send_mail($callback_settings, $order = array()) {
  global $language;
  foreach (array('callback_mail_body', 'callback_mail_subject', 'callback_mail_bcc', 'callback_mail_to', 'callback_mail_from') as $val) {
    $$val = $callback_settings['callback_mail_settings'][$val];
    $$val = !empty($order) ? token_replace($$val, array('commerce-order' => $order), array('clear' => TRUE)) : $$val;
  }
  $message = $mail_body = $callback_mail_body . LINEBREAK . $callback_settings['mail_body'];
  if (isset($callback_settings['greater_notify'])) {
    $message = $message . '<br>' . $callback_settings['greater_notify'];
  }

  if ($callback_settings['callback_mail_send']) {
    $params = array(
      'body'    => $mail_body,
      'subject' => $callback_mail_subject,
      'headers' => (!empty($callback_mail_bcc)) ? array('Bcc' => $callback_mail_bcc) : array(),
    );

    $mailresult = drupal_mail('commerce_novalnet', '', $callback_mail_to, $language->language, $params, $callback_mail_from, TRUE);
    $message = ($mailresult['result']) ? t('<br/>Mail Sent Successfully<br/>@msg', array('@msg' => $message)) : t('<br/>Mail not Sent<br/>@msg', array('@msg' => $message));
  }
  else {
    $message = t('Please configure callback mail to send mail<br/>Mail Content : @msg', array('@msg' => $message));
  }
  commerce_novalnet_show_callback_message($callback_settings, $message);
}

/**
 * Function to add transaction for credit entry.
 *
 * @param $request
 * @param $callback_settings
 * @param $payment_method
 * @param $order
 * @param $paid_amount
 */
function commerce_novalnet_add_credit_transaction($request, $callback_settings, $payment_method, $order, $paid_amount) {
  commerce_novalnet_create_new_transaction(array(
    'order_id'            => $order->order_id,
    'amount'              => $request['amount'],
    'currency'            => $request['currency'],
    'tid'                 => $request['tid'],
    'trans_status'        => $request['tid_status'],
    'reference_id'        => NULL,
    'payment_id'          => $payment_method['method_id'],
    'instance_id'         => $payment_method['instance_id'],
    'message'             => array('@message' => $callback_settings['mail_body']),
    'failure'             => !empty($request['failure_status']),
  // Call this function for create new transaction.
  ));
  $message = array(
    'order_id' => $order->order_id,
    'message'  => $callback_settings['mail_body'],
  );
  commerce_novalnet_add_order_history($message);

  db_update('commerce_novalnet_transaction_detail')
    ->fields(array('paid_amount' => $paid_amount))
    ->condition('tid', $request['org_tid'])
    ->condition('order_id', $order->order_id)
    ->execute();
}