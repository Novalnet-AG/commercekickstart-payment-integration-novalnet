/**
 * @file
 * Script for commerce_novalnet module.
 */

(function ($) {
  Drupal.behaviors.commerceNovalnetAdmin = {
    attach: function (context, settings) {
      $('#edit-refund-amount, #edit-amount-update, #edit-book-amount').keypress(function (event) {
        var keycode = ('which' in event) ? event.which : event.keyCode;
        var reg = /^(?:[0-9]+$)/;
        return (reg.test(String.fromCharCode(keycode)) || keycode == 0 || keycode == 8);
      });
      $('#edit-refund-ac-holder, #edit-refund-ac-iban, #edit-refund-ac-bic').keypress(function (event) {
        var keycode = ('which' in event) ? event.which : event.keyCode;
        var reg = /[\[\]\/\\#,+@!^()$~%'"=:;<>{}\_\|\d*?`°§]/g;
        if (this.id != 'edit-refund-ac-holder') {
            reg = /[\[\]\/\\#,+@!^()$~%-.&'"=:;<>{}\_\|\s*?°§`]/g;
        }
        return !reg.test(String.fromCharCode(keycode));
      });
      $('.form-item-cancel-options').hide();
      $('input[name=status]').live('click', function () {
        $('.form-item-cancel-options').hide();
        if ($(this).val() == 2) {
          $('.form-item-cancel-options').show();
        }
      });

      if ($('input#transaction_status', context).size() > 0) {
        var form = $('input#transaction_status', context).parents('form');
        $('input[type=submit]', form).click(function (event) {

          if ($('#edit-change-status-action a span').text() == Drupal.t('Hide')) {
            $('#novalnet_change_status_process').attr('value', 'novalnet_change_status_process');
            var selected_value = $('#edit-change-status').val();
            if (selected_value == 100) {
                return confirm(Drupal.t('Are you sure you want to capture the payment?'));
            }
            else if (selected_value == 103) {
                return confirm(Drupal.t('Are you sure you want to cancel the payment?'));
            }
          }
          if ($('#edit-refund-action a span').text() == Drupal.t('Hide')) {
            $('#novalnet_amount_refund_process').attr('value', 'novalnet_amount_refund_process');
            if ($('fieldset#edit-refund-action', context).size() > 0) {
              var refund_update = $.trim($('#edit-refund-amount').val());
              if (isNaN(refund_update) || refund_update == '') {
                alert(Drupal.t('The amount is invalid'));
                return false;
              }
              if (refund_update > 0) {
                return confirm(Drupal.t('Are you sure you want to refund the amount?'));
              }
            }
          }
          if ($('#edit-amount-update-action a span').text() == Drupal.t('Hide')) {
            $('#novalnet_amount_update_process').attr('value', 'novalnet_amount_update_process');

            if ($('fieldset#edit-amount-update-action', context).size() > 0) {
              var message = '';
              var amount_update = $.trim($('#edit-amount-update').val());

              if (amount_update == 0 || isNaN(amount_update)) {
                alert(Drupal.t('The amount is invalid'));
                return false;
              }
              if ($('#old_amount').val() != amount_update) {
                message = Drupal.t('Are you sure you want to change the order amount?');
              }
              if ($('#edit-due-date', context).size() > 0) {
                if ($('#edit-due-date-datepicker-popup-0').val() != $('#old_due_date').val()) {
                  message = Drupal.t('Are you sure you want to change the order amount or due date?');
                }
              }
              if (message != '') {
                return confirm(message);
              }
            }
          }
        });
      }
      $('#nn-global-submit').live('click', function (e) {
        if ($('.form-item-ajax-public-key .ajax-processed').hasClass('progress-disabled')) {
          e.preventDefault();
          $(document).ajaxComplete(function () {
            $('.form-item-ajax-public-key').closest("form").submit();
          });
        }
      });
    }
  }
})(jQuery);

if (typeof jQuery.fn.live == 'undefined' || !(jQuery.isFunction(jQuery.fn.live))) {
  jQuery.fn.extend({
      live: function (event, callback) {
         if (this.selector) {
              jQuery(document).on(event, this.selector, callback);
          }
      }
  });
}
