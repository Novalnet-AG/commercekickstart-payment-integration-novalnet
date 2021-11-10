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
  *  Script : commerce_novalnet.admin.inc
  */

/**
 * Novalnet global configuration form submit callback.
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_novalnet_global_config_form_submit($form, &$form_state) {
  $global_setting = $form_state['values']['ajax'];
  $global_setting['callback_notify_url'] = $form_state['values']['callback_config']['callback_notify_url'];

  if (isset($_SESSION['novalnet']['merchant_details'])) {
    $global_setting['normal_tariffs'] = $_SESSION['novalnet']['merchant_details']['normal_tariff'];
    $global_setting['subscription_tariffs'] = $_SESSION['novalnet']['merchant_details']['subscription_tariff'];
    unset($_SESSION['novalnet']['merchant_details']);
  }
  else {
    $merchant_details = variable_get('novalnet_global_settings', '');
    $global_setting['normal_tariffs'] = $merchant_details['normal_tariffs'];
    $global_setting['subscription_tariffs'] = $merchant_details['subscription_tariffs'];
  }

  drupal_set_message(t('The configuration options have been saved.'));

  variable_set('novalnet_callback_settings', $form_state['values']['callback_config']);
  variable_set('novalnet_global_settings', $global_setting);
}

/**
 * Add tariff options dynamically when the Novalnet public key is changed.
 *
 * @param array $form
 * @param array $form_state
 *
 * @return array
 */
function commerce_novalnet_ajax_callback(&$form, &$form_state) {
  $process_key = isset($form_state['values']) ? $form_state['values']['ajax']['public_key'] : $form['global_config']['ajax']['public_key']['#default_value'];
  // Call this function for Get Novalnet merchant details from Novalnet server for the $process_key.
  $merchant_details = commerce_novalnet_get_merchant_details($process_key);
  // If the merchant details available for $process_key.
  if ($merchant_details['status'] == 100) {
    $form['global_config']['ajax']['tariff_id']['#options']                = $merchant_details['normal_tariff'];
    $form['global_config']['ajax']['subs']['subs_tariff']['#options']      = $merchant_details['subscription_tariff'];
    $form['global_config']['ajax']['vendor_id']['#value']          = $merchant_details['vendor_id'];
    $form['global_config']['ajax']['auth_code']['#value']          = $merchant_details['auth_code'];
    $form['global_config']['ajax']['product_id']['#value']         = $merchant_details['product_id'];
    $form['global_config']['ajax']['payment_access_key']['#value'] = $merchant_details['access_key'];
    $_SESSION['novalnet']['merchant_details'] = $merchant_details;
  }
  // If the merchant details not available for $process_key.
  else {
    drupal_set_message($merchant_details['status_desc'], 'error', FALSE);
    $form['global_config']['ajax']['tariff_id']['#options']        = array();
    $form['global_config']['ajax']['subs']['subs_tariff']['#options']        = array();
    $form['global_config']['ajax']['vendor_id']['#value']          = '';
    $form['global_config']['ajax']['auth_code']['#value']          = '';
    $form['global_config']['ajax']['product_id']['#value']         = '';
    $form['global_config']['ajax']['payment_access_key']['#value'] = '';
  }
  return $form['global_config']['ajax'];
}

/**
 * Validate payment reference.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_payment_ref_validation($element, &$form_state) {
  $payment_ref = array_filter($form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings']['payment_references']);
  if (empty($payment_ref)) {
    form_error($element, t('Please select atleast one payment reference'));
  }
}

/**
 * Function for validate Refund reference.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_refund_ref($element, &$form_state) {
  $values = $form_state['values'];
  if (!empty($values['refund_amount']) && empty($element['#value'])) {
    form_error($element, t('@name field is required.', array('@name' => $element['#title'])));
  }
}

/**
 * Function for validate subscription cancel options.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_cancel_options($element, &$form_state) {
  $values = $form_state['values'];
  if ((!isset($values['status']) || (isset($values['status']) && $values['status'] == 2)) &&  empty($values['cancel_options'])) {
    form_error($element, t('@name field is required.', array('@name' => $element['#title'])));
  }
}

/**
 * Function for validate fraud module input field (mobile number)
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_fraud_data($element, &$form_state) {
  $value = trim($element['#value']);
  if (empty($form_state['values']['commerce_payment']['payment_details']['from_mask'])) {
    if (empty($value)) {
      form_error($element, t('@name field is required.', array('@name' => str_replace('*', '', filter_xss($element['#title'])))));
    }
    elseif (preg_match('/[a-zA-Z]/', $value)) {
      form_error($element, t('Invalid @name', array('@name' => str_replace('*', '', filter_xss($element['#title'])))));
    }
  }
}

/**
 * Function for validate fraud module pin field.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_pin($element, &$form_state) {
  $value = trim($element['#value']);
  if (empty($form_state['values']['commerce_payment']['payment_details']['nn_fraud_resend'])) {
    if (empty($value)) {
      form_error($element, t('@name field is required.', array('@name' => str_replace('*', '', filter_xss($element['#title'])))));
    }
    elseif (preg_match('/[\'\/~`\!@#\$%\^&\*\(\)_\-\+=\{\}\[\]\|;:"\<\>,\.\?\\\]/', $value)) {
      form_error($element, t('Invalid @name', array('@name' => str_replace('*', '', filter_xss($element['#title'])))));
    }
  }
}

/**
 * Function for validate email address.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_email($element, &$form_state) {
  $emails = explode(',', $element['#value']);
  if (($element['#name'] == 'callback_config[callback_mail_settings][callback_mail_bcc]' && !empty($emails[0])) || $element['#name'] != 'callback_config[callback_mail_settings][callback_mail_bcc]') {
    foreach ($emails as $email) {
      if (($form_state['values']['callback_config']['callback_mail_send'] == 1) && !valid_email_address($email) && !in_array($email, array('[site:mail]', '[commerce-order:mail]'))) {
        form_error($element, t('Invalid @title', array('@title' => $element['#title'])));
      }
    }
  }
}

/**
 * Function for validate date of birth.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_dob($element, &$form_state) {
  $dob_time_stamp = strtotime($element['#value']['year'] . '/' . $element['#value']['month'] . '/' . $element['#value']['day']);
  if ($dob_time_stamp > time()) {
    form_error($element, t('Invalid @title', array('@title' => $element['#title'])));
  }
}

/**
 * Function for validate due date.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_duedate($element, &$form_state) {
  $duedate_time_stamp = strtotime($element['#value']['year'] . '/' . $element['#value']['month'] . '/' . $element['#value']['day']);
  if ($duedate_time_stamp < strtotime($form_state['values']['old_due_date'])) {
    form_error($element, t('Invalid @title', array('@title' => $element['#title'])));
  }
}

/**
 * Function for perform transaction status update.
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_novalnet_transaction_status_update($form, &$form_state) {
  if (empty($form_state['values']['change_status']) || empty($form_state['values']['novalnet_change_status_process'])) {
    return TRUE;
  }

  $data = array(
    'status'      => $form_state['values']['change_status'],
    'edit_status' => 1,
  );
  // Call this function for update transaction details table.
  commerce_novalnet_transaction_details_update($data, $form_state['commerce_order'], 'status_update');
}

/**
 * Function for perform transaction amount update.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_transaction_amount_update($form, &$form_state) {
  $old_amount = $form_state['values']['old_amount'];
  $new_amount = $form_state['values']['amount_update'];
  $amount_change   = ($old_amount != $new_amount && $new_amount > 0);
  if (isset($form_state['values']['due_date_unformated'])) {
    $form_state['values']['due_date'] = $form_state['values']['due_date_unformated']['year'] . '-' . sprintf('%02d', $form_state['values']['due_date_unformated']['month']) . '-' . sprintf('%02d', $form_state['values']['due_date_unformated']['day']);
  }
  $due_date_change = (!empty($form_state['values']['due_date']) && $form_state['values']['due_date'] != $form_state['values']['old_due_date']);
  if (!$amount_change && !$due_date_change && empty($form_state['values']['novalnet_amount_update_process'])) {
    return TRUE;
  }

  $data = array(
    'status'      => 100,
    'edit_status' => 1,
    'update_inv_amount' => 1,
    'amount'      => $new_amount,
  );
  if ($due_date_change) {
    $data['due_date'] = $form_state['values']['due_date'];
  }
  // Call this function for update transaction details table.
  commerce_novalnet_transaction_details_update($data, $form_state['commerce_order'], 'amount_update');
}

/**
 * Function for perform the amount refund process.
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_novalnet_transaction_amount_refund($form, &$form_state) {
  if (empty($form_state['values']['refund_amount']) || (int) $form_state['values']['refund_amount'] == 0 || empty($form_state['values']['novalnet_amount_refund_process'])) {
    return TRUE;
  }

  $data = array(
    'refund_request' => '1',
    'refund_param'   => $form_state['values']['refund_amount'],
  );

  if (!empty($form_state['values']['refund_ref'])) {
    $data['refund_ref'] = $form_state['values']['refund_ref'];
  }

  // Call this function for update transaction details table.
  commerce_novalnet_transaction_details_update($data, $form_state['commerce_order'], 'amount_refund');

}

/**
 * Function for perform the amount booking process for zero amount transactions.
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_novalnet_transaction_zero_amount_booking($form, &$form_state) {

  if ($form_state['values']['book_amount'] > 0) {
    $order = $form_state['commerce_order'];
    $data = array();
    $payment_method = commerce_payment_method_instance_load($order->data['payment_method']);
    $method_id = strstr($order->data['payment_method'], '|', 1);
    $payment_method['global_setting'] = commerce_novalnet_validate_basic_data($method_id);

    commerce_novalnet_get_common_params($data, $payment_method, $order);

    $trans_details = commerce_novalnet_order_merchant_details($order->order_id, $order->data['tid']);

    $data = array_merge($data, unserialize($trans_details['vendor_details']));
    $data['payment_ref'] = $order->data['tid'];
    $data['amount'] = $form_state['values']['book_amount'];
    $data['key'] = $trans_details['payment_key'];

    $payment_details      = unserialize($trans_details['payment_details']);

    $data['payment_type'] = $payment_details['payment_type'];

    if (preg_match('/GUARANTEED/', $data['payment_type'])) {
      $data['birth_date']   = $payment_details['birth_date'];
    }

    $response = commerce_novalnet_drupal_request($data, COMMERCE_NOVALNET_PAYGATE_URL);

    parse_str($response, $parsed);
    $parsed = array_merge($data, $parsed);
    if ($parsed['status'] == 100) {
      $parsed['amount']     = str_replace('.', '', $parsed['amount']);
      $parsed['org_amount'] = $parsed['amount'];
      $trans_message = t('Your order has been booked with the amount of @amount. Your new TID for the booked amount: @tid', array('@amount' => commerce_currency_format($parsed['amount'], $parsed['currency']), '@tid' => $parsed['tid']));

      // Store Transaction details to the commerce_novalnet_transaction_detail table.
      commerce_novalnet_store_transaction($parsed, $payment_method);
      commerce_novalnet_create_new_transaction(array(
        'order_id'            => $order->order_id,
        'amount'              => $parsed['amount'],
        'currency'            => $parsed['currency'],
        'tid'                 => $parsed['tid'],
        'trans_status'        => $parsed['tid_status'],
        'reference'           => $parsed['payment_ref'],
        'payment_id'          => $payment_method['method_id'],
        'instance_id'         => $payment_method['instance_id'],
        'message'             => array('@message' => $trans_message),
      // Call this function for creating transaction for the particular order in shop.
      ));

      $order->data['tid'] = $parsed['tid'];
      $order->data['transaction_details'][] = array('update_on' => time(), 'message' => $trans_message);
      drupal_set_message($trans_message);
      commerce_order_save($order);
    }
    else {
      drupal_set_message($parsed['status_text'], 'error');
    }
  }
}

/**
 * Function for update transaction details table.
 *
 * @param array $data
 * @param object $order
 * @param string $update_type
 */
function commerce_novalnet_transaction_details_update($data, $order, $update_type) {
  $tid      = $order->data['tid'];
  $order_id = $order->order_id;

  $trans_details   = commerce_novalnet_order_merchant_details($order_id, $tid);
  $payment_details = unserialize($trans_details['payment_details']);
  $data['tid']         = $tid;
  $data['key']         = $trans_details['payment_key'];

  $data = array_merge($data, unserialize($trans_details['vendor_details']));
  $response = commerce_novalnet_drupal_request($data, COMMERCE_NOVALNET_PAYGATE_URL);
  parse_str($response, $parse_response);
  if ($parse_response['status'] == 100) {
    $global_setting = variable_get('novalnet_global_settings');
    $order_wrapper  = entity_metadata_wrapper('commerce_order', $order);
    $currency_code  = $order_wrapper->commerce_order_total->currency_code->value();
    $payment_method = commerce_payment_method_instance_load($order->data['payment_method']);

    switch ($update_type) {
      case 'status_update':

        if (!empty($data['key']) && in_array($data['key'], array('27', '41')) && $data['status'] == 100) {
          $message_text = t('The transaction has been confirmed successfully for the TID: @tid and the due date updated as @date', array('@tid' => $data['tid'], '@date' => $parse_response['due_date']));
          $message = commerce_novalnet_regenerate_comments($payment_method, $payment_details, $order, $data, $parse_response, $trans_details, $parse_response['due_date']);
          commerce_novalnet_add_extension_order_history(array('order_id' => $order->order_id, 'message' => $message)); 
          commerce_novalnet_transaction_confirmation_mail($message, $order);
        }
        else {
          $message_text = t('The transaction has been confirmed on @date @time.', array('@date' => date('Y-m-d'), '@time' => date('h:i:s')));
        }

        $status       = $global_setting['onhold_transaction_status'];
        $trans_data   = array(
          'order_id'            => $order->order_id,
          'amount'              => ($data['key'] == 27) ? $trans_details['paid_amount'] : $trans_details['org_total'],
          'currency'            => $currency_code,
          'tid'                 => $data['tid'],
          'trans_status'        => $parse_response['tid_status'],
          'reference_id'        => NULL,
          'payment_id'          => $payment_method['method_id'],
          'instance_id'         => $payment_method['instance_id'],
          'message'             => array('@message' => $message_text),
        );
        if ($data['status'] != 100) {
          $message_text = t('The transaction has been canceled on @date @time.', array('@date' => date('Y-m-d'), '@time' => date('h:i:s')));
          $status       = $global_setting['onhold_void_cancel_status'];
          $order->data['subs']['cancel'] = TRUE;
          $trans_data['failure'] = TRUE;
          $trans_data['message'] = array('@message' => $message_text);
          commerce_order_save($order);
        }
        if (!empty($parse_response['paypal_transaction_id'])) {
          $payment_details['paypal_transaction_id'] = $parse_response['paypal_transaction_id'];
        }
        commerce_novalnet_create_new_transaction($trans_data);
        // Core function for update the order status.
        commerce_order_status_update($order, $status, FALSE, TRUE, t('Order state updated'));
        $due_date = (!empty($data['key']) && in_array($data['key'], array('27', '41')) && $data['status'] == 100) ? strtotime($parse_response['due_date']) : time();
        $db_fields = array(
          'status'          => $data['status'],
          'changed'         => $due_date,
          'paid_amount'     => $trans_data['amount'],
          'booked'          => ($data['status'] == 100) ? 1 : 0,
          'payment_details' => serialize($payment_details),
        );
        break;

      case 'amount_update':      
        if((!empty($data['key']) && in_array($data['key'], array('27', '41')))) {
			$old_comments = commerce_novalnet_regenerate_comments($payment_method, $payment_details, $order, $data, $parse_response, $trans_details, $payment_details['due_date']);			
		}
		else {
			$old_comments = $order->data['transaction_details'][0]['message'];
		} 

        $amount = commerce_currency_format($data['amount'], $order->commerce_order_total['und'][0]['currency_code']);

        $updated_comments = '';
        $message_text   = t('The transaction amount @amount has been updated successfully on @date @time.', array('@amount' => commerce_currency_format($data['amount'], $currency_code), '@date' => date('Y-m-d'), '@time' => date('h:i:s'))) . '<br><br>' . $updated_comments;

        if ($trans_details['payment_type'] != 'commerce_novalnet_sepa') {
          $payment_details = unserialize($trans_details['payment_details']);
          $updated_comments = preg_replace('|<span id="amount">(.+?)</span>|s', $amount, $old_comments);
          $due_date = !empty($data['due_date']) ? $data['due_date'] : $payment_details['due_date'];
          $formated_due_date = date('m/d/Y', strtotime($due_date));
          $updated_comments = preg_replace('|<span id="due_date">(.+?)</span>|s', $formated_due_date , $updated_comments);

          $update_array = array(
                            '@amount' => commerce_currency_format($data['amount'], $currency_code),
                            '@due_date' => $formated_due_date
                           );

          $message_text   = ($trans_details['payment_type'] == 'commerce_novalnet_barzahlen') ? t('The transaction has been updated with amount @amount and slip expiry date with @due_date', $update_array ) : t('The transaction has been updated with amount @amount and due date with @due_date', $update_array );
          $message_text .= '<br><br>' . $updated_comments;
        }

        $db_fields = array(
          'org_total'   => $data['amount'],
          'changed'     => time(),
          'paid_amount' => ($trans_details['payment_type'] == 'commerce_novalnet_sepa') ? $data['amount'] : $trans_details['paid_amount'],
        );

        if (isset($data['due_date'])) {
          $payment_details              = unserialize($trans_details['payment_details']);
          $payment_details['due_date']  = $data['due_date'];
          $db_fields['payment_details'] = serialize($payment_details);
        }

        break;

      case 'amount_refund':
        $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
        $currency_code = $order_wrapper->commerce_order_total->currency_code->value();
        $message_text  = t('The refund has been executed for the TID: @tid with the amount of @amount.', array('@tid' => $data['tid'], '@amount' => commerce_currency_format($data['refund_param'], $currency_code)));

        if (isset($parse_response['tid'])) {
          $message_text .= ' ' . t('Your new TID for the refund amount: @ref_tid', array('@ref_tid' => $parse_response['tid']));
        }
        elseif (isset($parse_response['paypal_refund_tid'])) {
          $message_text .= ' ' . t('Your new TID for the refund amount: @ref_tid', array('@ref_tid' => $parse_response['paypal_refund_tid']));
        }

        if (isset($parse_response['tid_status'])) {
          $db_fields = array('status' => $parse_response['tid_status']);
          if ($parse_response['tid_status'] == '103') {
            // Core function for update the order status.
            commerce_order_status_update($order, $global_setting['onhold_void_cancel_status'], FALSE, TRUE, t('Order state updated'));
            if (isset($order->data['subs'])) {
              $order->data['subs']['cancel'] = TRUE;
              commerce_order_save($order);
            }
          }
        }

        break;
    }

    $message = array(
      'order_id' => $order->order_id,
      'message'  => $message_text,
    );
    commerce_novalnet_add_extension_order_history($message); 

    $db_update = db_update('commerce_novalnet_transaction_detail');

    if ($update_type == 'amount_refund') {
      $db_update->expression('refunded_amount', 'refunded_amount + :refunded_amount', array(':refunded_amount' => $data['refund_param']));
    }
    if (!empty($db_fields)) {
      $db_update->fields($db_fields);
    }
    $db_update->condition('tid', $data['tid'])
      ->condition('order_id', $order->order_id)
      ->execute();
  }
  else {
    $status_text = commerce_novalnet_get_status_text($parse_response);
    drupal_set_message(check_plain($status_text), 'error');
  }
}

/**
 * Function for add message on order history.
 *
 * @param array $msg
 * @param $show bool
 */
function commerce_novalnet_add_order_history($msg, $show = TRUE) {
  $order = commerce_order_load($msg['order_id']);
  $order->data['transaction_details'][] = array('update_on' => time(), 'message' => $msg['message']);
  commerce_order_save($order);

  if ($show) {
    drupal_set_message(check_plain($msg['message']));
  }
}

/**
 * Function for add message on extension process order history.
 *
 * @param array $msg
 * @param $show bool
 */
function commerce_novalnet_add_extension_order_history($msg, $show = TRUE) {
  $order = commerce_order_load($msg['order_id']);
  $order->data['transaction_details'][] = array('update_on' => time(), 'message' => $msg['message']);
  commerce_order_save($order);

  if ($show) {
    drupal_set_message($msg['message']);
  }
}

/**
 * Function perform subscription cancel.
 *
 * @param array $form
 * @param array $form_state
 */
function commerce_novalnet_recurring_cancel_form_submit($form, &$form_state) {
  $recurring_orders = $form_state['build_info']['args'][1]->commerce_recurring_order['und'];
  $order_id = $recurring_orders[0]['target_id'];
  $order = commerce_order_load($order_id);
  $tid   = $order->data['tid'];
  $merchant_details = commerce_novalnet_order_merchant_details($order_id, $tid);
  $vendor_details = unserialize($merchant_details['vendor_details']);
  $reason = $form_state['values']['cancel_options'];
  $cancel_request = array(
    'vendor' => $vendor_details['vendor'],
    'product' => $vendor_details['product'],
    'tariff' => $vendor_details['tariff'],
    'key' => $merchant_details['payment_key'],
    'auth_code' => $vendor_details['auth_code'],
    'cancel_sub' => 1,
    'tid' => $tid,
    'cancel_reason' => $reason,
  );
  return commerce_novalnet_recurring_cancel_request($cancel_request, $recurring_orders, $order);

}

/**
 * Function for send subscription cancel request.
 *
 * @param array $cancel_request
 * @param object $recurring_orders
 * @param array $order
 *
 * @return bool
 */
function commerce_novalnet_recurring_cancel_request($cancel_request, $recurring_orders, $order) {

  $response = commerce_novalnet_drupal_request($cancel_request, COMMERCE_NOVALNET_PAYGATE_URL);

  parse_str($response, $response);

  if ($response['status'] == 100) {
    $order->data['subs']['cancel'] = TRUE;
    commerce_order_save($order);

    $global_setting = variable_get('novalnet_global_settings', '');

    $message = array(
      'order_id' => $order->order_id,
      'message'  => t('Subscription has been canceled due to: @reason', array('@reason' => $cancel_request['cancel_reason'])),
    );
    commerce_novalnet_add_order_history($message, TRUE);

    foreach ($recurring_orders as $orders) {
      // Core function for update the order status.
      commerce_order_status_update(commerce_order_load($orders['target_id']), $global_setting['subs']['cancel_status'], FALSE, TRUE, t('Order state updated'));
    }

  }
  else {
    drupal_set_message($response['status_desc'], 'error');
  }
  return TRUE;

}

/**
 * Function for process subscription updates.
 *
 * @param array $form
 * @param array $form_state
 *
 * @return bool
 */
function commerce_novalnet_recurring_update_submit($form, &$form_state) {
  $order_id = $form['#entity']->commerce_recurring_order['und'][0]['target_id'];

  $order = commerce_order_load($order_id);
  $order_details = commerce_novalnet_order_merchant_details($order->order_id, $order->data['tid']);
  $vendor_details = unserialize($order_details['vendor_details']);
  $vendor_data = array(
    'vendor_id'       => $vendor_details['vendor'],
    'vendor_authcode' => $vendor_details['auth_code'],
    'product_id'      => $vendor_details['product'],
  );

  $current_due_date = date('Y-m-d', $form['#entity']->due_date);
  $updated_due_date = date('Y-m-d', $form_state['values']['due_date']);
  if ($current_due_date != $updated_due_date) {
    $current_due_date = new DateTime($current_due_date);
    $updated_due_date = new DateTime($updated_due_date);
    $difference = $updated_due_date->diff($current_due_date);
    $data = array_merge($vendor_data, array(
      'request_type'    => 'SUBSCRIPTION_PAUSE',
      'tid'             => $order->data['tid'],
      'subs_id'         => $order->data['subs']['subs_id'],
      'pause_period'    => $difference->days,
      'pause_time_unit' => 'd',
    ));
    $response = commerce_novalnet_drupal_request($data, COMMERCE_NOVALNET_XML_URL, TRUE);
    $response = (array) simplexml_load_string($response);
    if (isset($response['status']) && $response['status'] == 100) {
      $message = t('Subscription renewal date has been successfully changed');
      commerce_novalnet_add_order_history(array('order_id' => $order_id, 'message' => $message));
    }
    else {
    form_set_error('status', check_plain((string) $response['subscription_pause']->status_message));      //drupal_set_message(check_plain((string) $response['subscription_pause']->status_message), 'error');
      return FALSE;
    }
  }

  if ($form['#entity']->status != $form_state['values']['status']) {

    if ($form_state['values']['status'] == 2) {
      $reason = ($form_state['values']['cancel_options'] == 'Other') ? $form_state['values']['others_options'] : $form_state['values']['cancel_options'];
      $cancel_request = array(
        'vendor'     => $vendor_details['vendor'],
        'product'    => $vendor_details['product'],
        'tariff'     => $vendor_details['tariff'],
        'key'        => $order_details['payment_key'],
        'auth_code'  => $vendor_details['auth_code'],
        'cancel_sub' => 1,
        'tid'        => $order->data['tid'],
        'cancel_reason' => $reason,
      );
      commerce_novalnet_recurring_cancel_request($cancel_request, $form['#entity']->commerce_recurring_order['und'], $order);
      db_update('commerce_recurring')->fields(array('status' => 0))->condition('id', $form['#entity']->id)->execute();
      return TRUE;
    }
    else {
      $data = array_merge($vendor_data, array(
        'request_type'    => 'SUBSCRIPTION_PAUSE',
        'tid'             => $order->data['tid'],
        'subs_id'         => $order->data['subs']['subs_id'],
        'suspend'         => 1,
      ));
      $message = t('This subscription transaction has been suspended on @date', array('@date' => date('Y-m-d h:i:s')));
      $order->data['subs']['status'] = 'suspended';
      $order->data['subs']['cancel'] = FALSE;

      if ($form_state['values']['status']) {
        $order->data['subs']['status'] = 'active';
        $product_id = $form_state['commerce_recurring']->commerce_recurring_ref_product['und'][0]['target_id'];
        $product    = commerce_product_load($product_id);
        $data['suspend']         = '0';
        $rec_interval = $product->commerce_recurring_rec_period['und'][0];
        $message = t('Subscription has been successfully activated on @date', array('@date' => date('Y-m-d h:i:s')));

        if ($rec_interval['period'] == 'week') {
          $rec_interval = array(
            'period'   => 'day',
            'interval' => $rec_interval['interval'] * 7,
          );
        }
        $pre_next_date   = !empty($order->data['subs']['next_cycle_date']) ? date('Y-m-d', $order->data['subs']['next_cycle_date']) : date('Y-m-d', $form['#entity']->due_date);
        $current_date    = date('Y-m-d');
        $current_date    = '2017-08-25';
        if ($pre_next_date <= $current_date) {
          $next_cycle    = date('Y-m-d', strtotime('+' . $rec_interval['interval'] . ' ' . $rec_interval['period'], strtotime($pre_next_date)));
          while ($next_cycle <= $current_date) {
            $next_cycle    = date('Y-m-d', strtotime('+' . $rec_interval['interval'] . ' ' . $rec_interval['period'], strtotime($next_cycle)));
          }
          $pre_next_date = new DateTime($pre_next_date);
          $next_cycle = new DateTime($next_cycle);
          $difference = $pre_next_date->diff($next_cycle);
          $data['pause_period']    = $difference->days;
          $data['pause_time_unit'] = 'd';
        }
      }
      $response = commerce_novalnet_drupal_request($data, COMMERCE_NOVALNET_XML_URL, TRUE);
      $response = (array) simplexml_load_string($response);
      if (isset($response['status']) && $response['status'] == 100) {
        commerce_novalnet_add_order_history(array('order_id' => $order_id, 'message' => $message));
      }
      else {
        form_set_error('status', check_plain((string) $response['subscription_pause']->status_message));
        return FALSE;
      }
    }
  }

  return FALSE;
}

/**
 * Function for validate due date.
 *
 * @param array $element
 * @param array $form_state
 */
function commerce_novalnet_element_validate_required($element, &$form_state) {
  if (isset($element['#value']['date'])) {
    $value = trim($element['#value']['date']);
  }
  else {
    $value = trim($element['#value']);
  }

  if (empty($value)) {
    form_error($element, t('@name field is required.', array('@name' => $element['#title'])));
  }
}


function commerce_novalnet_show_updateinfo() {
    return theme('update_info');
}

function commerce_novalnet_regenerate_comments($payment_method, $payment_details, $order, $data, $parse_response, $trans_details, $due_date) {	
	  $message = '';
	  $txn_message = '';
	  $gurantee_info = '';
	  if ($data['key'] == 41) {
		  $gurantee_info = t('This is processed as a guarantee payment');
	  } 
	  $currency_type = $order->commerce_order_total['und'][0]['currency_code'];
	  $trans_array = array(
		'@test'   		 => (!empty($order->data['test_mode'])) ? t('Test order') : '',
		'@payment_name'  => t('Invoice'),
		'@tid'   		 => $data['tid'],
	  );
	  $due_date = isset($due_date) ? $due_date : $parse_response['due_date'];
	  $txn_message = '<br>' . t('@test<br>Payment Method: @payment_name<br>Novalnet transaction ID: @tid', $trans_array) .'<br>';
	  $account_info  = '<br>' . t('Please transfer the amount to the below mentioned account details of our payment processor Novalnet') . '<br>';
	  $account_info .= t('Due date') . ': <span id="due_date">' . date('m/d/Y', strtotime($due_date)) . '</span><br>' . t('Account holder') . ': ' . t('Novalnet AG') . '<br>' . t('IBAN') . ': ' . $payment_details['invoice_iban'] . '<br>' . t('BIC') . ': ' . $payment_details['invoice_bic'] . '<br>' . t('Bank') . ': ' . $payment_details['invoice_bankname'] . ' ' . $payment_details['invoice_bankplace'] . '<br>' . t('Amount') . ': <span id="amount">' . commerce_currency_format($trans_details['org_total'], $currency_type) . '</span><br><br>';

	  $references_settings = $payment_method['settings']['payment_references'];
	  $payment_details['invoice_ref'] = isset($payment_details['invoice_ref']) ? $payment_details['invoice_ref'] : '';

	  $payment_ref_array = array_filter(
			array(
			  $payment_details['invoice_ref']             => $references_settings['payment_ref_1'],
			  t('TID') . ' ' . $data['tid']               => $references_settings['payment_ref_2'],
			  t('Order number') . ' ' . $order->order_id  => $references_settings['payment_ref_3'],
			)
	  );

	  $ref_count = count($payment_ref_array);
	  $ref_info = (($ref_count > 1) ? t('Please use any one of the following references as the payment reference, as only through this way your payment is matched and assigned to the order:') : t('Please use the following payment reference for your money transfer, as only through this way your payment is matched and assigned to the order:')) . '<br>';
	  $i = 1;

	  foreach ($payment_ref_array as $key => $val) {
		  $ref_info .= (($ref_count > 1) ? t('Payment Reference @no', array('@no' => $i)) : t('Payment Reference')) . ': ' . $key . '<br>';
		  $i++;
	  }
	  return $message = $gurantee_info. $txn_message. '<div id="prepayment_comments">' . $account_info . $ref_info . '</div>';
}
