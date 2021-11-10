/**
* @file
* Script for commerce_novalnet_cc module.
*/

(function ($) {
  Drupal.behaviors.commerceNovalnetCC = {
    attach: function (context, settings) {
    var iframewindow = false;
    loadIframe = function () {
      iframewindow = get_iframe_content();
      $(window).resize(function () {
        iframewindow.postMessage({'callBack'    : 'getHeight'}, "https://secure.novalnet.de");
      });
      var cc_style = (settings.cc_style !== undefined) ? settings.cc_style : ''
      var messageObj = {
        'callBack'    : 'createElements',
        'customStyle' : {
          labelStyle : (cc_style.label_style !== undefined) ? cc_style.label_style : '',
          inputStyle : (cc_style.field_style !== undefined) ? cc_style.field_style : '',
          styleText  : (cc_style.style_text !== undefined) ? cc_style.style_text : '',
          card_holder : {
            labelStyle : (cc_style.cc_holder.label_style !== undefined) ? cc_style.cc_holder.label_style : '',
            inputStyle : (cc_style.cc_holder.field_style !== undefined) ? cc_style.cc_holder.field_style : '',
          },
          card_number : {
            labelStyle : (cc_style.cc_number.label_style !== undefined) ? cc_style.cc_number.label_style : '',
            inputStyle : (cc_style.cc_number.field_style !== undefined) ? cc_style.cc_number.field_style : '',
          },
          expiry_date : {
            labelStyle : (cc_style.cc_expiry.label_style !== undefined) ? cc_style.cc_expiry.label_style : '',
            inputStyle : (cc_style.cc_expiry.field_style !== undefined) ? cc_style.cc_expiry.field_style : '',
          },
          cvc       : {
            labelStyle : (cc_style.cc_cvc.label_style !== undefined) ? cc_style.cc_cvc.label_style : '',
            inputStyle : (cc_style.cc_cvc.field_style !== undefined) ? cc_style.cc_cvc.field_style : '',
          }
        },
        'customText'  : {
          card_holder: {
            labelText: Drupal.t('Card holder name'),
            inputText: Drupal.t('Name on card'),
          },
          card_number: {
            labelText: Drupal.t('Card number'),
            inputText: Drupal.t('XXXX XXXX XXXX XXXX'),
          },
          expiry_date: {
            labelText: Drupal.t('Expiry date'),
            inputText: Drupal.t('MM / YYYY'),
          },
          cvc: {
            labelText: Drupal.t('CVC/CVV/CID'),
            inputText: Drupal.t('XXX'),
          },
          cvcHintText: Drupal.t('what is this?'),
          errorText: Drupal.t('Your credit card details are invalid'),
        }
      }
      iframewindow.postMessage(messageObj, "https://secure.novalnet.de");
    }

    if ($('#nncc_normal_form', context).size() > 0) {
      $('#nncc_normal_form, .redirect_desc, #nncc_error').hide();
      if ($('#nncc_mask_form').size() <= 0) {
        $('#nncc_normal_form').show();
      }

      var form = $('#nncc_normal_form', context).parents('form');
      $('#change_mask_cc').live('click', function (event) {
        event.stopImmediatePropagation();
        event.preventDefault();
        if ($('#from_cc_mask').attr('value') == 1) {
          hide_cc_mask();
        }
        else {
          show_cc_mask();
        }
      });

      $(window).bind("message", function (e) {
        var orgEvent = e.originalEvent;
        var data = $.parseJSON(orgEvent.data);
        if (orgEvent.origin == 'https://secure.novalnet.de' && data.callBack != undefined) {
          if (data.callBack == 'getHeight') {
            $('#nn_iframe').height(data.contentHeight);
          }
          else if (data.callBack == 'getHash') {
            $('#nncc_error').hide();
            if (data.result == undefined || data.result != 'success') {
              var error_message = (data.error_message != undefined) ? data.error_message : Drupal.t('Your credit card details are invalid');
              $('#nncc_error').html(error_message).show();
            }
            else {
              if (data.hash == undefined || data.hash == undefined) {
                $('#nncc_error').html(Drupal.t('Your credit card details are invalid')).show();
              }
              else {
                $('#nncc_hash').val(data.hash);
                $('#nncc_uniquid').val(data.unique_id);
                $(form).submit();
              }
            }
          }
        }
      });

      $(form).once('commerceNovalnetCC', function () {
        $('input[type=submit], #edit-submit', form).click(function (event) {
          if (this.id == 'edit-back') {
            return true;
          }
          if ($("input[name='commerce_payment[payment_method]']:checked").attr('value') == "commerce_novalnet_cc|commerce_payment_commerce_novalnet_cc" || $("input[name='commerce_payment[payment_method]']").size() == 0) {
              if (!$(form).hasClass('novalnet-cc-processed')) {
                event.stopImmediatePropagation();
                event.preventDefault();
                if ($('#from_cc_mask').size() <= 0 || $('#from_cc_mask').val() == 0) {
                  iframewindow = get_iframe_content();
                  $(window).resize(function () {
                    iframewindow.postMessage({'callBack'    : 'getHeight'}, "https://secure.novalnet.de");
                  });
                  iframewindow.postMessage({'callBack' : 'getHash'}, "https://secure.novalnet.de");
                }
                else {
                  $(form).submit();
                }
              }
            }
            return true;
          });
        });
      }
    }
  };

  function get_iframe_content() {
    var iframe = $('#nn_iframe')[0];
    return iframe.contentWindow ? iframe.contentWindow : iframe.contentDocument.defaultView;
  }

  function show_cc_mask() {
    $('#nncc_mask_form').show();
    $('#nncc_normal_form').hide();
    $('#from_cc_mask').attr('value', 1);
    $('#change_mask_cc').html(Drupal.t('Enter new card details'));
  }

  function hide_cc_mask() {
    $('#nncc_mask_form').hide();
    $('#nncc_normal_form').show();
    iframewindow = get_iframe_content();
    $(window).resize(function () {
      iframewindow.postMessage({'callBack'    : 'getHeight'}, "https://secure.novalnet.de");
    });
    iframewindow.postMessage({'callBack' : 'getHeight'}, "https://secure.novalnet.de");
    $('#from_cc_mask').attr('value', 0);
    $('#change_mask_cc').html(Drupal.t('Given card details'));
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
