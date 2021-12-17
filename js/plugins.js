jQuery(document).ready(function($){
  var slidesWrapper = $('.cd-hero-slider');

  //check if a .cd-hero-slider exists in the DOM 
  if ( slidesWrapper.length > 0 ) {
    var primaryNav = $('.cd-primary-nav'),
      sliderNav = $('.cd-slider-nav'),
      navigationMarker = $('.cd-marker'),
      slidesNumber = slidesWrapper.children('li').length,
      visibleSlidePosition = 0,
      autoPlayId,
      autoPlayDelay = 5000;

    //upload videos (if not on mobile devices)
    uploadVideo(slidesWrapper);

    //autoplay slider
    setAutoplay(slidesWrapper, slidesNumber, autoPlayDelay);

    //on mobile - open/close primary navigation clicking/tapping the menu icon
    primaryNav.on('click', function(event){
      if($(event.target).is('.cd-primary-nav')) $(this).children('ul').toggleClass('is-visible');
    });
    
    //change visible slide
    sliderNav.on('click', 'li', function(event){
      event.preventDefault();
      var selectedItem = $(this);
      if(!selectedItem.hasClass('selected')) {
        // if it's not already selected
        var selectedPosition = selectedItem.index(),
          activePosition = slidesWrapper.find('li.selected').index();
        
        if( activePosition < selectedPosition) {
          nextSlide(slidesWrapper.find('.selected'), slidesWrapper, sliderNav, selectedPosition);
        } else {
          prevSlide(slidesWrapper.find('.selected'), slidesWrapper, sliderNav, selectedPosition);
        }

        //this is used for the autoplay
        visibleSlidePosition = selectedPosition;

        updateSliderNavigation(sliderNav, selectedPosition);
        updateNavigationMarker(navigationMarker, selectedPosition+1);
        //reset autoplay
        setAutoplay(slidesWrapper, slidesNumber, autoPlayDelay);
      }
    });
  }

  function nextSlide(visibleSlide, container, pagination, n){
    visibleSlide.removeClass('selected from-left from-right').addClass('is-moving').one('webkitTransitionEnd otransitionend oTransitionEnd msTransitionEnd transitionend', function(){
      visibleSlide.removeClass('is-moving');
    });

    container.children('li').eq(n).addClass('selected from-right').prevAll().addClass('move-left');
    checkVideo(visibleSlide, container, n);
  }

  function prevSlide(visibleSlide, container, pagination, n){
    visibleSlide.removeClass('selected from-left from-right').addClass('is-moving').one('webkitTransitionEnd otransitionend oTransitionEnd msTransitionEnd transitionend', function(){
      visibleSlide.removeClass('is-moving');
    });

    container.children('li').eq(n).addClass('selected from-left').removeClass('move-left').nextAll().removeClass('move-left');
    checkVideo(visibleSlide, container, n);
  }

  function updateSliderNavigation(pagination, n) {
    var navigationDot = pagination.find('.selected');
    navigationDot.removeClass('selected');
    pagination.find('li').eq(n).addClass('selected');
  }

  function setAutoplay(wrapper, length, delay) {
    if(wrapper.hasClass('autoplay')) {
      clearInterval(autoPlayId);
      autoPlayId = window.setInterval(function(){autoplaySlider(length)}, delay);
    }
  }

  function autoplaySlider(length) {
    if( visibleSlidePosition < length - 1) {
      nextSlide(slidesWrapper.find('.selected'), slidesWrapper, sliderNav, visibleSlidePosition + 1);
      visibleSlidePosition +=1;
    } else {
      prevSlide(slidesWrapper.find('.selected'), slidesWrapper, sliderNav, 0);
      visibleSlidePosition = 0;
    }
    updateNavigationMarker(navigationMarker, visibleSlidePosition+1);
    updateSliderNavigation(sliderNav, visibleSlidePosition);
  }

  function uploadVideo(container) {
    container.find('.cd-bg-video-wrapper').each(function(){
      var videoWrapper = $(this);
      if( videoWrapper.is(':visible') ) {
        // if visible - we are not on a mobile device 
        var videoUrl = videoWrapper.data('video'),
          video = $('<video loop><source src="'+videoUrl+'.mp4" type="video/mp4" /><source src="'+videoUrl+'.webm" type="video/webm" /></video>');
        video.appendTo(videoWrapper);
        // play video if first slide
        if(videoWrapper.parent('.cd-bg-video.selected').length > 0) video.get(0).play();
      }
    });
  }

  function checkVideo(hiddenSlide, container, n) {
    //check if a video outside the viewport is playing - if yes, pause it
    var hiddenVideo = hiddenSlide.find('video');
    if( hiddenVideo.length > 0 ) hiddenVideo.get(0).pause();

    //check if the select slide contains a video element - if yes, play the video
    var visibleVideo = container.children('li').eq(n).find('video');
    if( visibleVideo.length > 0 ) visibleVideo.get(0).play();
  }

  function updateNavigationMarker(marker, n) {
    marker.removeClassPrefix('item').addClass('item-'+n);
  }

  $.fn.removeClassPrefix = function(prefix) {
    //remove all classes starting with 'prefix'
      this.each(function(i, el) {
          var classes = el.className.split(" ").filter(function(c) {
              return c.lastIndexOf(prefix, 0) !== 0;
          });
          el.className = $.trim(classes.join(" "));
      });
      return this;
  };
});
/*!
 * Lightbox v2.9.0
 * by Lokesh Dhakar
 *
 * More info:
 * http://lokeshdhakar.com/projects/lightbox2/
 *
 * Copyright 2007, 2015 Lokesh Dhakar
 * Released under the MIT license
 * https://github.com/lokesh/lightbox2/blob/master/LICENSE
 */

// Uses Node, AMD or browser globals to create a module.
(function (root, factory) {
    if (typeof define === 'function' && define.amd) {
        // AMD. Register as an anonymous module.
        define(['jquery'], factory);
    } else if (typeof exports === 'object') {
        // Node. Does not work with strict CommonJS, but
        // only CommonJS-like environments that support module.exports,
        // like Node.
        module.exports = factory(require('jquery'));
    } else {
        // Browser globals (root is window)
        root.lightbox = factory(root.jQuery);
    }
}(this, function ($) {

  function Lightbox(options) {
    this.album = [];
    this.currentImageIndex = void 0;
    this.init();

    // options
    this.options = $.extend({}, this.constructor.defaults);
    this.option(options);
  }

  // Descriptions of all options available on the demo site:
  // http://lokeshdhakar.com/projects/lightbox2/index.html#options
  Lightbox.defaults = {
    albumLabel: 'Image %1 of %2',
    alwaysShowNavOnTouchDevices: false,
    fadeDuration: 600,
    fitImagesInViewport: true,
    imageFadeDuration: 600,
    // maxWidth: 800,
    // maxHeight: 600,
    positionFromTop: 50,
    resizeDuration: 700,
    showImageNumberLabel: true,
    wrapAround: false,
    disableScrolling: false,
    /*
    Sanitize Title
    If the caption data is trusted, for example you are hardcoding it in, then leave this to false.
    This will free you to add html tags, such as links, in the caption.

    If the caption data is user submitted or from some other untrusted source, then set this to true
    to prevent xss and other injection attacks.
     */
    sanitizeTitle: false
  };

  Lightbox.prototype.option = function(options) {
    $.extend(this.options, options);
  };

  Lightbox.prototype.imageCountLabel = function(currentImageNum, totalImages) {
    return this.options.albumLabel.replace(/%1/g, currentImageNum).replace(/%2/g, totalImages);
  };

  Lightbox.prototype.init = function() {
    var self = this;
    // Both enable and build methods require the body tag to be in the DOM.
    $(document).ready(function() {
      self.enable();
      self.build();
    });
  };

  // Loop through anchors and areamaps looking for either data-lightbox attributes or rel attributes
  // that contain 'lightbox'. When these are clicked, start lightbox.
  Lightbox.prototype.enable = function() {
    var self = this;
    $('body').on('click', 'a[rel^=lightbox], area[rel^=lightbox], a[data-lightbox], area[data-lightbox]', function(event) {
      self.start($(event.currentTarget));
      return false;
    });
  };

  // Build html for the lightbox and the overlay.
  // Attach event handlers to the new DOM elements. click click click
  Lightbox.prototype.build = function() {
    var self = this;
    $('<div id="lightboxOverlay" class="lightboxOverlay"></div><div id="lightbox" class="lightbox"><div class="lb-outerContainer"><div class="lb-container"><img class="lb-image" src="data:image/gif;base64,R0lGODlhAQABAIAAAP///wAAACH5BAEAAAAALAAAAAABAAEAAAICRAEAOw==" /><div class="lb-nav"><a class="lb-prev" href="" ></a><a class="lb-next" href="" ></a></div><div class="lb-loader"><a class="lb-cancel"></a></div></div></div><div class="lb-dataContainer"><div class="lb-data"><div class="lb-details"><span class="lb-caption"></span><span class="lb-number"></span></div><div class="lb-closeContainer"><a class="lb-close"></a></div></div></div></div>').appendTo($('body'));

    // Cache jQuery objects
    this.$lightbox       = $('#lightbox');
    this.$overlay        = $('#lightboxOverlay');
    this.$outerContainer = this.$lightbox.find('.lb-outerContainer');
    this.$container      = this.$lightbox.find('.lb-container');
    this.$image          = this.$lightbox.find('.lb-image');
    this.$nav            = this.$lightbox.find('.lb-nav');

    // Store css values for future lookup
    this.containerPadding = {
      top: parseInt(this.$container.css('padding-top'), 10),
      right: parseInt(this.$container.css('padding-right'), 10),
      bottom: parseInt(this.$container.css('padding-bottom'), 10),
      left: parseInt(this.$container.css('padding-left'), 10)
    };

    this.imageBorderWidth = {
      top: parseInt(this.$image.css('border-top-width'), 10),
      right: parseInt(this.$image.css('border-right-width'), 10),
      bottom: parseInt(this.$image.css('border-bottom-width'), 10),
      left: parseInt(this.$image.css('border-left-width'), 10)
    };

    // Attach event handlers to the newly minted DOM elements
    this.$overlay.hide().on('click', function() {
      self.end();
      return false;
    });

    this.$lightbox.hide().on('click', function(event) {
      if ($(event.target).attr('id') === 'lightbox') {
        self.end();
      }
      return false;
    });

    this.$outerContainer.on('click', function(event) {
      if ($(event.target).attr('id') === 'lightbox') {
        self.end();
      }
      return false;
    });

    this.$lightbox.find('.lb-prev').on('click', function() {
      if (self.currentImageIndex === 0) {
        self.changeImage(self.album.length - 1);
      } else {
        self.changeImage(self.currentImageIndex - 1);
      }
      return false;
    });

    this.$lightbox.find('.lb-next').on('click', function() {
      if (self.currentImageIndex === self.album.length - 1) {
        self.changeImage(0);
      } else {
        self.changeImage(self.currentImageIndex + 1);
      }
      return false;
    });

    /*
      Show context menu for image on right-click

      There is a div containing the navigation that spans the entire image and lives above of it. If
      you right-click, you are right clicking this div and not the image. This prevents users from
      saving the image or using other context menu actions with the image.

      To fix this, when we detect the right mouse button is pressed down, but not yet clicked, we
      set pointer-events to none on the nav div. This is so that the upcoming right-click event on
      the next mouseup will bubble down to the image. Once the right-click/contextmenu event occurs
      we set the pointer events back to auto for the nav div so it can capture hover and left-click
      events as usual.
     */
    this.$nav.on('mousedown', function(event) {
      if (event.which === 3) {
        self.$nav.css('pointer-events', 'none');

        self.$lightbox.one('contextmenu', function() {
          setTimeout(function() {
              this.$nav.css('pointer-events', 'auto');
          }.bind(self), 0);
        });
      }
    });


    this.$lightbox.find('.lb-loader, .lb-close').on('click', function() {
      self.end();
      return false;
    });
  };

  // Show overlay and lightbox. If the image is part of a set, add siblings to album array.
  Lightbox.prototype.start = function($link) {
    var self    = this;
    var $window = $(window);

    $window.on('resize', $.proxy(this.sizeOverlay, this));

    $('select, object, embed').css({
      visibility: 'hidden'
    });

    this.sizeOverlay();

    this.album = [];
    var imageNumber = 0;

    function addToAlbum($link) {
      self.album.push({
        link: $link.attr('href'),
        title: $link.attr('data-title') || $link.attr('title')
      });
    }

    // Support both data-lightbox attribute and rel attribute implementations
    var dataLightboxValue = $link.attr('data-lightbox');
    var $links;

    if (dataLightboxValue) {
      $links = $($link.prop('tagName') + '[data-lightbox="' + dataLightboxValue + '"]');
      for (var i = 0; i < $links.length; i = ++i) {
        addToAlbum($($links[i]));
        if ($links[i] === $link[0]) {
          imageNumber = i;
        }
      }
    } else {
      if ($link.attr('rel') === 'lightbox') {
        // If image is not part of a set
        addToAlbum($link);
      } else {
        // If image is part of a set
        $links = $($link.prop('tagName') + '[rel="' + $link.attr('rel') + '"]');
        for (var j = 0; j < $links.length; j = ++j) {
          addToAlbum($($links[j]));
          if ($links[j] === $link[0]) {
            imageNumber = j;
          }
        }
      }
    }

    // Position Lightbox
    var top  = $window.scrollTop() + this.options.positionFromTop;
    var left = $window.scrollLeft();
    this.$lightbox.css({
      top: top + 'px',
      left: left + 'px'
    }).fadeIn(this.options.fadeDuration);

    // Disable scrolling of the page while open
    if (this.options.disableScrolling) {
      $('body').addClass('lb-disable-scrolling');
    }

    this.changeImage(imageNumber);
  };

  // Hide most UI elements in preparation for the animated resizing of the lightbox.
  Lightbox.prototype.changeImage = function(imageNumber) {
    var self = this;

    this.disableKeyboardNav();
    var $image = this.$lightbox.find('.lb-image');

    this.$overlay.fadeIn(this.options.fadeDuration);

    $('.lb-loader').fadeIn('slow');
    this.$lightbox.find('.lb-image, .lb-nav, .lb-prev, .lb-next, .lb-dataContainer, .lb-numbers, .lb-caption').hide();

    this.$outerContainer.addClass('animating');

    // When image to show is preloaded, we send the width and height to sizeContainer()
    var preloader = new Image();
    preloader.onload = function() {
      var $preloader;
      var imageHeight;
      var imageWidth;
      var maxImageHeight;
      var maxImageWidth;
      var windowHeight;
      var windowWidth;

      $image.attr('src', self.album[imageNumber].link);

      $preloader = $(preloader);

      $image.width(preloader.width);
      $image.height(preloader.height);

      if (self.options.fitImagesInViewport) {
        // Fit image inside the viewport.
        // Take into account the border around the image and an additional 10px gutter on each side.

        windowWidth    = $(window).width();
        windowHeight   = $(window).height();
        maxImageWidth  = windowWidth - self.containerPadding.left - self.containerPadding.right - self.imageBorderWidth.left - self.imageBorderWidth.right - 20;
        maxImageHeight = windowHeight - self.containerPadding.top - self.containerPadding.bottom - self.imageBorderWidth.top - self.imageBorderWidth.bottom - 120;

        // Check if image size is larger then maxWidth|maxHeight in settings
        if (self.options.maxWidth && self.options.maxWidth < maxImageWidth) {
          maxImageWidth = self.options.maxWidth;
        }
        if (self.options.maxHeight && self.options.maxHeight < maxImageWidth) {
          maxImageHeight = self.options.maxHeight;
        }

        // Is there a fitting issue?
        if ((preloader.width > maxImageWidth) || (preloader.height > maxImageHeight)) {
          if ((preloader.width / maxImageWidth) > (preloader.height / maxImageHeight)) {
            imageWidth  = maxImageWidth;
            imageHeight = parseInt(preloader.height / (preloader.width / imageWidth), 10);
            $image.width(imageWidth);
            $image.height(imageHeight);
          } else {
            imageHeight = maxImageHeight;
            imageWidth = parseInt(preloader.width / (preloader.height / imageHeight), 10);
            $image.width(imageWidth);
            $image.height(imageHeight);
          }
        }
      }
      self.sizeContainer($image.width(), $image.height());
    };

    preloader.src          = this.album[imageNumber].link;
    this.currentImageIndex = imageNumber;
  };

  // Stretch overlay to fit the viewport
  Lightbox.prototype.sizeOverlay = function() {
    this.$overlay
      .width($(document).width())
      .height($(document).height());
  };

  // Animate the size of the lightbox to fit the image we are showing
  Lightbox.prototype.sizeContainer = function(imageWidth, imageHeight) {
    var self = this;

    var oldWidth  = this.$outerContainer.outerWidth();
    var oldHeight = this.$outerContainer.outerHeight();
    var newWidth  = imageWidth + this.containerPadding.left + this.containerPadding.right + this.imageBorderWidth.left + this.imageBorderWidth.right;
    var newHeight = imageHeight + this.containerPadding.top + this.containerPadding.bottom + this.imageBorderWidth.top + this.imageBorderWidth.bottom;

    function postResize() {
      self.$lightbox.find('.lb-dataContainer').width(newWidth);
      self.$lightbox.find('.lb-prevLink').height(newHeight);
      self.$lightbox.find('.lb-nextLink').height(newHeight);
      self.showImage();
    }

    if (oldWidth !== newWidth || oldHeight !== newHeight) {
      this.$outerContainer.animate({
        width: newWidth,
        height: newHeight
      }, this.options.resizeDuration, 'swing', function() {
        postResize();
      });
    } else {
      postResize();
    }
  };

  // Display the image and its details and begin preload neighboring images.
  Lightbox.prototype.showImage = function() {
    this.$lightbox.find('.lb-loader').stop(true).hide();
    this.$lightbox.find('.lb-image').fadeIn(this.options.imageFadeDuration);

    this.updateNav();
    this.updateDetails();
    this.preloadNeighboringImages();
    this.enableKeyboardNav();
  };

  // Display previous and next navigation if appropriate.
  Lightbox.prototype.updateNav = function() {
    // Check to see if the browser supports touch events. If so, we take the conservative approach
    // and assume that mouse hover events are not supported and always show prev/next navigation
    // arrows in image sets.
    var alwaysShowNav = false;
    try {
      document.createEvent('TouchEvent');
      alwaysShowNav = (this.options.alwaysShowNavOnTouchDevices) ? true : false;
    } catch (e) {}

    this.$lightbox.find('.lb-nav').show();

    if (this.album.length > 1) {
      if (this.options.wrapAround) {
        if (alwaysShowNav) {
          this.$lightbox.find('.lb-prev, .lb-next').css('opacity', '1');
        }
        this.$lightbox.find('.lb-prev, .lb-next').show();
      } else {
        if (this.currentImageIndex > 0) {
          this.$lightbox.find('.lb-prev').show();
          if (alwaysShowNav) {
            this.$lightbox.find('.lb-prev').css('opacity', '1');
          }
        }
        if (this.currentImageIndex < this.album.length - 1) {
          this.$lightbox.find('.lb-next').show();
          if (alwaysShowNav) {
            this.$lightbox.find('.lb-next').css('opacity', '1');
          }
        }
      }
    }
  };

  // Display caption, image number, and closing button.
  Lightbox.prototype.updateDetails = function() {
    var self = this;

    // Enable anchor clicks in the injected caption html.
    // Thanks Nate Wright for the fix. @https://github.com/NateWr
    if (typeof this.album[this.currentImageIndex].title !== 'undefined' &&
      this.album[this.currentImageIndex].title !== '') {
      var $caption = this.$lightbox.find('.lb-caption');
      if (this.options.sanitizeTitle) {
        $caption.text(this.album[this.currentImageIndex].title);
      } else {
        $caption.html(this.album[this.currentImageIndex].title);
      }
      $caption.fadeIn('fast')
        .find('a').on('click', function(event) {
          if ($(this).attr('target') !== undefined) {
            window.open($(this).attr('href'), $(this).attr('target'));
          } else {
            location.href = $(this).attr('href');
          }
        });
    }

    if (this.album.length > 1 && this.options.showImageNumberLabel) {
      var labelText = this.imageCountLabel(this.currentImageIndex + 1, this.album.length);
      this.$lightbox.find('.lb-number').text(labelText).fadeIn('fast');
    } else {
      this.$lightbox.find('.lb-number').hide();
    }

    this.$outerContainer.removeClass('animating');

    this.$lightbox.find('.lb-dataContainer').fadeIn(this.options.resizeDuration, function() {
      return self.sizeOverlay();
    });
  };

  // Preload previous and next images in set.
  Lightbox.prototype.preloadNeighboringImages = function() {
    if (this.album.length > this.currentImageIndex + 1) {
      var preloadNext = new Image();
      preloadNext.src = this.album[this.currentImageIndex + 1].link;
    }
    if (this.currentImageIndex > 0) {
      var preloadPrev = new Image();
      preloadPrev.src = this.album[this.currentImageIndex - 1].link;
    }
  };

  Lightbox.prototype.enableKeyboardNav = function() {
    $(document).on('keyup.keyboard', $.proxy(this.keyboardAction, this));
  };

  Lightbox.prototype.disableKeyboardNav = function() {
    $(document).off('.keyboard');
  };

  Lightbox.prototype.keyboardAction = function(event) {
    var KEYCODE_ESC        = 27;
    var KEYCODE_LEFTARROW  = 37;
    var KEYCODE_RIGHTARROW = 39;

    var keycode = event.keyCode;
    var key     = String.fromCharCode(keycode).toLowerCase();
    if (keycode === KEYCODE_ESC || key.match(/x|o|c/)) {
      this.end();
    } else if (key === 'p' || keycode === KEYCODE_LEFTARROW) {
      if (this.currentImageIndex !== 0) {
        this.changeImage(this.currentImageIndex - 1);
      } else if (this.options.wrapAround && this.album.length > 1) {
        this.changeImage(this.album.length - 1);
      }
    } else if (key === 'n' || keycode === KEYCODE_RIGHTARROW) {
      if (this.currentImageIndex !== this.album.length - 1) {
        this.changeImage(this.currentImageIndex + 1);
      } else if (this.options.wrapAround && this.album.length > 1) {
        this.changeImage(0);
      }
    }
  };

  // Closing time. :-(
  Lightbox.prototype.end = function() {
    this.disableKeyboardNav();
    $(window).off('resize', this.sizeOverlay);
    this.$lightbox.fadeOut(this.options.fadeDuration);
    this.$overlay.fadeOut(this.options.fadeDuration);
    $('select, object, embed').css({
      visibility: 'visible'
    });
    if (this.options.disableScrolling) {
      $('body').removeClass('lb-disable-scrolling');
    }
  };

  return new Lightbox();
}));


/*
* MIXITUP - A CSS3 and JQuery Filter & Sort Plugin
* Version: 1.5.5
* License: Creative Commons Attribution-NoDerivs 3.0 Unported - CC BY-ND 3.0
* http://creativecommons.org/licenses/by-nd/3.0/
* This software may be used freely on commercial and non-commercial projects with attribution to the author/copyright holder.
* Author: Patrick Kunka
* Copyright 2012-2013 Patrick Kunka, Barrel LLC, All Rights Reserved
* 
* http://mixitup.io
*/

(function($){
    
    // DECLARE METHODS
 
    var methods = {

        // "INIT" METHOD
    
        init: function(settings){

            return this.each(function(){
                
                var browser = window.navigator.appVersion.match(/Chrome\/(\d+)\./),
                    ver = browser ? parseInt(browser[1], 10) : false,
                    chromeFix = function(id){
                        var grid = document.getElementById(id),
                            parent = grid.parentElement,
                            placeholder = document.createElement('div'),
                            frag = document.createDocumentFragment();

                        parent.insertBefore(placeholder, grid);  
                        frag.appendChild(grid);
                        parent.replaceChild(grid, placeholder);
                        frag = null;
                        placeholder = null;
                    };
                
                if(ver && ver == 31 || ver == 32){
                    chromeFix(this.id);
                };
                
                // BUILD CONFIG OBJECT

                var config = {
                    
                    // PUBLIC PROPERTIES
                    
                    targetSelector : '.mix',
                    filterSelector : '.filter',
                    sortSelector : '.sort',
                    buttonEvent: 'click',
                    effects : ['fade', 'scale'],
                    listEffects : null,
                    easing : 'smooth',
                    layoutMode: 'grid',
                    targetDisplayGrid : 'inline-block',
                    targetDisplayList: 'block',
                    listClass : '',
                    gridClass : '',
                    transitionSpeed : 600,
                    showOnLoad : 'all',
                    sortOnLoad : false,
                    multiFilter : false,
                    filterLogic : 'or',
                    resizeContainer : true,
                    minHeight : 0,
                    failClass : 'fail',
                    perspectiveDistance : '3000',
                    perspectiveOrigin : '50% 50%',
                    animateGridList : true,
                    onMixLoad: null,
                    onMixStart : null,
                    onMixEnd : null,

                    // MISC

                    container : null,
                    origOrder : [],
                    startOrder : [],
                    newOrder : [],
                    origSort: [],
                    checkSort: [],
                    filter : '',
                    mixing : false,
                    origDisplay : '',
                    origLayout: '',
                    origHeight : 0, 
                    newHeight : 0,
                    isTouch : false,
                    resetDelay : 0,
                    failsafe : null,

                    // CSS
                    
                    prefix : '',
                    easingFallback : 'ease-in-out',
                    transition : {}, 
                    perspective : {}, 
                    clean : {},
                    fade : '1',
                    scale : '',
                    rotateX : '',
                    rotateY : '',
                    rotateZ : '',
                    blur : '',
                    grayscale : ''
                };
                
                if(settings){
                    $.extend(config, settings);
                };

                // ADD CONFIG OBJECT TO CONTAINER OBJECT PER INSTANTIATION
                
                this.config = config;
                
                // DETECT TOUCH
                
                $.support.touch = 'ontouchend' in document;

                if ($.support.touch) {
                    config.isTouch = true;
                    config.resetDelay = 350;
                };
                
                // LOCALIZE CONTAINER
    
                config.container = $(this);
                var $cont = config.container;
                
                // GET VENDOR PREFIX
                
                config.prefix = prefix($cont[0]);
                config.prefix = config.prefix ? '-'+config.prefix.toLowerCase()+'-' : '';
                
                // CACHE 'DEFAULT' SORTING ORDER
            
                $cont.find(config.targetSelector).each(function(){
                    config.origOrder.push($(this));
                });
                
                // PERFORM SORT ON LOAD 
                
                if(config.sortOnLoad){
                    var sortby, order;
                    if($.isArray(config.sortOnLoad)){
                        sortby = config.sortOnLoad[0], order = config.sortOnLoad[1];
                        $(config.sortSelector+'[data-sort='+config.sortOnLoad[0]+'][data-order='+config.sortOnLoad[1]+']').addClass('active');
                    } else {
                        $(config.sortSelector+'[data-sort='+config.sortOnLoad+']').addClass('active');
                        sortby = config.sortOnLoad, config.sortOnLoad = 'desc';
                    };
                    sort(sortby, order, $cont, config);
                };
                
                // BUILD TRANSITION AND PERSPECTIVE OBJECTS
                
                for(var i = 0; i<2; i++){
                    var a = i==0 ? a = config.prefix : '';
                    config.transition[a+'transition'] = 'all '+config.transitionSpeed+'ms ease-in-out';
                    config.perspective[a+'perspective'] = config.perspectiveDistance+'px';
                    config.perspective[a+'perspective-origin'] = config.perspectiveOrigin;
                };
                
                // BUILD TRANSITION CLEANER
                
                for(var i = 0; i<2; i++){
                    var a = i==0 ? a = config.prefix : '';
                    config.clean[a+'transition'] = 'none';
                };
    
                // CHOOSE GRID OR LIST
    
                if(config.layoutMode == 'list'){
                    $cont.addClass(config.listClass);
                    config.origDisplay = config.targetDisplayList;
                } else {
                    $cont.addClass(config.gridClass);
                    config.origDisplay = config.targetDisplayGrid;
                };
                config.origLayout = config.layoutMode;
                
                // PARSE 'SHOWONLOAD'
                
                var showOnLoadArray = config.showOnLoad.split(' ');
                
                // GIVE ACTIVE FILTER ACTIVE CLASS
                
                $.each(showOnLoadArray, function(){
                    $(config.filterSelector+'[data-filter="'+this+'"]').addClass('active');
                });
                
                // RENAME "ALL" CATEGORY TO "MIX_ALL"
    
                $cont.find(config.targetSelector).addClass('mix_all');
                if(showOnLoadArray[0]  == 'all'){
                    showOnLoadArray[0] = 'mix_all',
                    config.showOnLoad = 'mix_all';
                };
                
                // FADE IN 'SHOWONLOAD'
                
                var $showOnLoad = $();
                $.each(showOnLoadArray, function(){
                    $showOnLoad = $showOnLoad.add($('.'+this))
                });
                
                $showOnLoad.each(function(){
                    var $t = $(this);
                    if(config.layoutMode == 'list'){
                        $t.css('display',config.targetDisplayList);
                    } else {
                        $t.css('display',config.targetDisplayGrid);
                    };
                    $t.css(config.transition);
                });
                
                // WRAP FADE-IN TO PREVENT RACE CONDITION
                
                var delay = setTimeout(function(){
                    
                    config.mixing = true;
                    
                    $showOnLoad.css('opacity','1');
                    
                    // CLEAN UP
                    
                    var reset = setTimeout(function(){
                        if(config.layoutMode == 'list'){
                            $showOnLoad.removeStyle(config.prefix+'transition, transition').css({
                                display: config.targetDisplayList,
                                opacity: 1
                            });
                        } else {
                            $showOnLoad.removeStyle(config.prefix+'transition, transition').css({
                                display: config.targetDisplayGrid,
                                opacity: 1
                            });
                        };
                        
                        // FIRE "ONMIXLOAD" CALLBACK
                        
                        config.mixing = false;

                        if(typeof config.onMixLoad == 'function') {
                            var output = config.onMixLoad.call(this, config);

                            // UPDATE CONFIG IF DATA RETURNED

                            config = output ? output : config;
                        };
                        
                    },config.transitionSpeed);
                },10);
                
                // PRESET ACTIVE FILTER
                
                config.filter = config.showOnLoad;
            
                // BIND SORT CLICK HANDLERS
            
                $(config.sortSelector).bind(config.buttonEvent,function(){
                    
                    if(!config.mixing){
                        
                        // PARSE SORT ARGUMENTS FROM BUTTON CLASSES
                        
                        var $t = $(this),
                        sortby = $t.attr('data-sort'),
                        order = $t.attr('data-order');
                        
                        if(!$t.hasClass('active')){
                            $(config.sortSelector).removeClass('active');
                            $t.addClass('active');
                        } else {
                            if(sortby != 'random')return false;
                        };
                        
                        $cont.find(config.targetSelector).each(function(){
                            config.startOrder.push($(this));
                        });
                
                        goMix(config.filter,sortby,order,$cont, config);
                
                    };
                
                });

                // BIND FILTER CLICK HANDLERS

                $(config.filterSelector).bind(config.buttonEvent,function(){
                
                    if(!config.mixing){
                        
                        var $t = $(this);
                        
                        // PARSE FILTER ARGUMENTS FROM BUTTON CLASSES
        
                        if(config.multiFilter == false){
                            
                            // SINGLE ACTIVE BUTTON
                            
                            $(config.filterSelector).removeClass('active');
                            $t.addClass('active');
                        
                            config.filter = $t.attr('data-filter');
                        
                            $(config.filterSelector+'[data-filter="'+config.filter+'"]').addClass('active');

                        } else {
                        
                            // MULTIPLE ACTIVE BUTTONS
                            
                            var thisFilter = $t.attr('data-filter'); 
                        
                            if($t.hasClass('active')){
                                $t.removeClass('active');
                                
                                // REMOVE FILTER FROM SPACE-SEPERATED STRING
                                
                                var re = new RegExp('(\\s|^)'+thisFilter);
                                config.filter = config.filter.replace(re,'');
                            } else {
                                
                                // ADD FILTER TO SPACE-SEPERATED STRING
                                
                                $t.addClass('active');
                                config.filter = config.filter+' '+thisFilter;
                                
                            };
                        };
                        
                        // GO MIX
                        
                        goMix(config.filter, null, null, $cont, config);

                    };
                
                });
                    
            });
        },
    
        // "TOGRID" METHOD
    
        toGrid: function(){
            return this.each(function(){
                var config = this.config;
                if(config.layoutMode != 'grid'){
                    config.layoutMode = 'grid';
                    goMix(config.filter, null, null, $(this), config);
                };
            });
        },
    
        // "TOLIST" METHOD
    
        toList: function(){
            return this.each(function(){
                var config = this.config;
                if(config.layoutMode != 'list'){
                    config.layoutMode = 'list';
                    goMix(config.filter, null, null, $(this), config);
                };
            });
        },
    
        // "FILTER" METHOD
    
        filter: function(arg){
            return this.each(function(){
                var config = this.config;
                if(!config.mixing){ 
                    $(config.filterSelector).removeClass('active');
                    $(config.filterSelector+'[data-filter="'+arg+'"]').addClass('active');
                    goMix(arg, null, null, $(this), config);
                };
            }); 
        },
    
        // "SORT" METHOD
    
        sort: function(args){
            return this.each(function(){
                var config = this.config,
                    $t = $(this);
                if(!config.mixing){
                    $(config.sortSelector).removeClass('active');
                    if($.isArray(args)){
                        var sortby = args[0], order = args[1];
                        $(config.sortSelector+'[data-sort="'+args[0]+'"][data-order="'+args[1]+'"]').addClass('active');
                    } else {
                        $(config.sortSelector+'[data-sort="'+args+'"]').addClass('active');
                        var sortby = args, order = 'desc';
                    };
                    $t.find(config.targetSelector).each(function(){
                        config.startOrder.push($(this));
                    });
                    
                    goMix(config.filter,sortby,order, $t, config);
                
                };
            });
        },
        
        // "MULTIMIX" METHOD
        
        multimix: function(args){
            return this.each(function(){
                var config = this.config,
                    $t = $(this);
                    multiOut = {
                        filter: config.filter,
                        sort: null,
                        order: 'desc',
                        layoutMode: config.layoutMode
                    };
                $.extend(multiOut, args);
                if(!config.mixing){
                    $(config.filterSelector).add(config.sortSelector).removeClass('active');
                    $(config.filterSelector+'[data-filter="'+multiOut.filter+'"]').addClass('active');
                    if(typeof multiOut.sort !== 'undefined'){
                        $(config.sortSelector+'[data-sort="'+multiOut.sort+'"][data-order="'+multiOut.order+'"]').addClass('active');
                        $t.find(config.targetSelector).each(function(){
                            config.startOrder.push($(this));
                        });
                    };
                    config.layoutMode = multiOut.layoutMode;
                    goMix(multiOut.filter,multiOut.sort,multiOut.order, $t, config);
                };
            });
        },
        
        // "REMIX" METHOD

        remix: function(arg){
            return this.each(function(){
                var config = this.config,
                    $t = $(this);   
                config.origOrder = [];
                $t.find(config.targetSelector).each(function(){
                    var $th = $(this);
                    $th.addClass('mix_all'); 
                    config.origOrder.push($th);
                });
                if(!config.mixing && typeof arg !== 'undefined'){
                    $(config.filterSelector).removeClass('active');
                    $(config.filterSelector+'[data-filter="'+arg+'"]').addClass('active');
                    goMix(arg, null, null, $t, config);
                };
            });
        }
    };
    
    // DECLARE PLUGIN

    $.fn.mixitup = function(method, arg){
        if (methods[method]) {
            return methods[method].apply( this, Array.prototype.slice.call(arguments,1));
        } else if (typeof method === 'object' || ! method){
            return methods.init.apply( this, arguments );
        };
    };
    
    /* ==== THE MAGIC ==== */
    
    function goMix(filter, sortby, order, $cont, config){
        
        // WE ARE NOW MIXING

        clearInterval(config.failsafe);
        config.mixing = true;   
        
        // APPLY ARGS TO CONFIG
        
        config.filter = filter;
        
        // FIRE "ONMIXSTART" CALLBACK
        
        if(typeof config.onMixStart == 'function') {
            var output = config.onMixStart.call(this, config);
            
            // UPDATE CONFIG IF DATA RETURNED
            
            config = output ? output : config;
        };
        
        // SHORT LOCAL VARS
        
        var speed = config.transitionSpeed;
        
        // REBUILD TRANSITION AND PERSPECTIVE OBJECTS
        
        for(var i = 0; i<2; i++){
            var a = i==0 ? a = config.prefix : '';
            config.transition[a+'transition'] = 'all '+speed+'ms linear';
            config.transition[a+'transform'] = a+'translate3d(0,0,0)';
            config.perspective[a+'perspective'] = config.perspectiveDistance+'px';
            config.perspective[a+'perspective-origin'] = config.perspectiveOrigin;
        };
        
        // CACHE TARGET ELEMENTS FOR QUICK ACCESS
        
        var mixSelector = config.targetSelector,
        $targets = $cont.find(mixSelector);
        
        // ADD DATA OBJECT TO EACH TARGET
        
        $targets.each(function(){
            this.data = {};
        });
        
        // RE-DEFINE CONTAINER INCASE NOT IMMEDIATE PARENT OF TARGET ELEMENTS 
        
        var $par = $targets.parent();
    
        // ADD PERSPECTIVE TO CONTAINER 
        
        $par.css(config.perspective);
        
        // SETUP EASING

        config.easingFallback = 'ease-in-out';
        if(config.easing == 'smooth')config.easing = 'cubic-bezier(0.25, 0.46, 0.45, 0.94)';
        if(config.easing == 'snap')config.easing = 'cubic-bezier(0.77, 0, 0.175, 1)';
        if(config.easing == 'windback'){
            config.easing = 'cubic-bezier(0.175, 0.885, 0.320, 1.275)',
            config.easingFallback = 'cubic-bezier(0.175, 0.885, 0.320, 1)'; // Fall-back for old webkit, with no values > 1 or < 1
        };
        if(config.easing == 'windup'){
            config.easing = 'cubic-bezier(0.6, -0.28, 0.735, 0.045)',
            config.easingFallback = 'cubic-bezier(0.6, 0.28, 0.735, 0.045)';
        };
        
        // USE LIST SPECIFIC EFFECTS IF DECLARED
        
        var effectsOut = config.layoutMode == 'list' && config.listEffects != null ? config.listEffects : config.effects;
    
        // BUILD EFFECTS STRINGS & SKIP IF IE8
    
        if (Array.prototype.indexOf){
            config.fade = effectsOut.indexOf('fade') > -1 ? '0' : '';
            config.scale = effectsOut.indexOf('scale') > -1 ? 'scale(.01)' : '';
            config.rotateZ = effectsOut.indexOf('rotateZ') > -1 ? 'rotate(180deg)' : '';
            config.rotateY = effectsOut.indexOf('rotateY') > -1 ? 'rotateY(90deg)' : '';
            config.rotateX = effectsOut.indexOf('rotateX') > -1 ? 'rotateX(90deg)' : '';
            config.blur = effectsOut.indexOf('blur') > -1 ? 'blur(8px)' : '';
            config.grayscale = effectsOut.indexOf('grayscale') > -1 ? 'grayscale(100%)' : '';
        };
        
        // DECLARE NEW JQUERY OBJECTS FOR GROUPING
        
        var $show = $(), 
        $hide = $(),
        filterArray = [],
        multiDimensional = false;
        
        // BUILD FILTER ARRAY(S)
        
        if(typeof filter === 'string'){
            
            // SINGLE DIMENSIONAL FILTERING
            
            filterArray = buildFilterArray(filter);
            
        } else {
            
            // MULTI DIMENSIONAL FILTERING
            
            multiDimensional = true;
            
            $.each(filter,function(i){
                filterArray[i] = buildFilterArray(this);
            });
        };

        // "OR" LOGIC (DEFAULT)
        
        if(config.filterLogic == 'or'){
            
            if(filterArray[0] == '') filterArray.shift(); // IF FIRST ITEM IN ARRAY IS AN EMPTY SPACE, DELETE
            
            // IF NO ELEMENTS ARE DESIRED THEN HIDE ALL VISIBLE ELEMENTS
        
            if(filterArray.length < 1){
                
                $hide = $hide.add($cont.find(mixSelector+':visible'));
                
            } else {

            // ELSE CHECK EACH TARGET ELEMENT FOR ANY FILTER CATEGORY:
            
                $targets.each(function(){
                    var $t = $(this);
                    if(!multiDimensional){
                        // IF HAS ANY FILTER, ADD TO "SHOW" OBJECT
                        if($t.is('.'+filterArray.join(', .'))){
                            $show = $show.add($t);
                        // ELSE IF HAS NO FILTERS, ADD TO "HIDE" OBJECT
                        } else {
                            $hide = $hide.add($t);
                        };
                    } else {
                        
                        var pass = 0;
                        // FOR EACH DIMENSION
                        
                        $.each(filterArray,function(i){
                            if(this.length){
                                if($t.is('.'+this.join(', .'))){
                                    pass++
                                };
                            } else if(pass > 0){
                                pass++;
                            };
                        });
                        // IF PASSES ALL DIMENSIONS, SHOW
                        if(pass == filterArray.length){
                            $show = $show.add($t);
                        // ELSE HIDE
                        } else {
                            $hide = $hide.add($t);
                        };
                    };
                });
            
            };
    
        } else {
            
        // "AND" LOGIC
            
            // ADD "MIX_SHOW" CLASS TO ELEMENTS THAT HAVE ALL FILTERS
            
            $show = $show.add($par.find(mixSelector+'.'+filterArray.join('.')));
            
            // ADD "MIX_HIDE" CLASS TO EVERYTHING ELSE
            
            $hide = $hide.add($par.find(mixSelector+':not(.'+filterArray.join('.')+'):visible'));
        };
        
        // GET TOTAL NUMBER OF ELEMENTS TO SHOW
        
        var total = $show.length;
        
        // DECLARE NEW JQUERY OBJECTS

        var $tohide = $(),
        $toshow = $(),
        $pre = $();
        
        // FOR ELEMENTS TO BE HIDDEN, IF NOT ALREADY HIDDEN THEN ADD TO OBJECTS "TOHIDE" AND "PRE" 
        // TO INDICATE PRE-EXISTING ELEMENTS TO BE HIDDEN
        
        $hide.each(function(){
            var $t = $(this);
            if($t.css('display') != 'none'){
                $tohide = $tohide.add($t);
                $pre = $pre.add($t);
            };
        });
        
        // IF ALL ELEMENTS ARE ALREADY SHOWN AND THERE IS NOTHING TO HIDE, AND NOT PERFORMING A LAYOUT CHANGE OR SORT:
        
        if($show.filter(':visible').length == total && !$tohide.length && !sortby){
            
            if(config.origLayout == config.layoutMode){
                
                // THEN CLEAN UP AND GO HOME

                resetFilter();
                return false;
            } else {
                
                // IF ONLY ONE ITEM AND CHANGING FORM GRID TO LIST, MOST LIKELY POSITION WILL NOT CHANGE SO WE'RE DONE
            
                if($show.length == 1){ 
                    
                    if(config.layoutMode == 'list'){ 
                        $cont.addClass(config.listClass);
                        $cont.removeClass(config.gridClass);
                        $pre.css('display',config.targetDisplayList);
                    } else {
                        $cont.addClass(config.gridClass);
                        $cont.removeClass(config.listClass);
                        $pre.css('display',config.targetDisplayGrid);
                    };
                    
                    // THEN CLEAN UP AND GO HOME

                    resetFilter();
                    return false;
                }
            };
        };
        
        // GET CONTAINER'S STARTING HEIGHT

        config.origHeight = $par.height();
        
        // IF THERE IS SOMETHING TO BE SHOWN:

        if($show.length){
            
            // REMOVE "FAIL CLASS" FROM CONTAINER IF EXISTS
            
            $cont.removeClass(config.failClass);
            
            
            // FOR ELEMENTS TO BE SHOWN, IF NOT ALREADY SHOWN THEN ADD TO OBJECTS "TOSHOW" ELSE ADD CLASS "MIX_PRE"
            // TO INDICATE PRE-EXISTING ELEMENT

            $show.each(function(){ 
                var $t = $(this);
                if($t.css('display') == 'none'){
                    $toshow = $toshow.add($t)
                } else {
                    $pre = $pre.add($t);
                };
            });
    
            // IF NON-ANIMATED LAYOUT MODE TRANSITION:
        
            if((config.origLayout != config.layoutMode) && config.animateGridList == false){ 
            
                // ADD NEW DISPLAY TYPES, CLEAN UP AND GO HOME
                
                if(config.layoutMode == 'list'){ 
                    $cont.addClass(config.listClass);
                    $cont.removeClass(config.gridClass);
                    $pre.css('display',config.targetDisplayList);
                } else {
                    $cont.addClass(config.gridClass);
                    $cont.removeClass(config.listClass);
                    $pre.css('display',config.targetDisplayGrid);
                };
                
                resetFilter();
                return false;
            };
            
            // IF IE, FUCK OFF, AND THEN CLEAN UP AND GO HOME
        
            if(!window.atob){
                resetFilter();
                return false;
            };
            
            // OVERRIDE ANY EXISTING TRANSITION TIMING FOR CALCULATIONS
            
            $targets.css(config.clean);
            
            // FOR EACH PRE-EXISTING ELEMENT, ADD STARTING POSITION TO 'ORIGPOS' ARRAY
            
            $pre.each(function(){
                this.data.origPos = $(this).offset();
            });
    
            // TEMPORARILY SHOW ALL ELEMENTS TO SHOW (THAT ARE NOT ALREADY SHOWN), WITHOUT HIDING ELEMENTS TO HIDE
            // AND ADD/REMOVE GRID AND LIST CLASSES FROM CONTAINER
    
            if(config.layoutMode == 'list'){
                $cont.addClass(config.listClass);
                $cont.removeClass(config.gridClass);
                $toshow.css('display',config.targetDisplayList);
            } else {
                $cont.addClass(config.gridClass);
                $cont.removeClass(config.listClass);
                $toshow.css('display',config.targetDisplayGrid);
            };
            
            // FOR EACH ELEMENT NOW SHOWN, ADD ITS INTERMEDIATE POSITION TO 'SHOWINTERPOS' ARRAY
    
            $toshow.each(function(){
                this.data.showInterPos = $(this).offset();
            });
            
            // FOR EACH ELEMENT TO BE HIDDEN, BUT NOT YET HIDDEN, AND NOW MOVED DUE TO SHOWN ELEMENTS,
            // ADD ITS INTERMEDIATE POSITION TO 'HIDEINTERPOS' ARRAY

            $tohide.each(function(){
                this.data.hideInterPos = $(this).offset();
            });
            
            // FOR EACH PRE-EXISTING ELEMENT, NOW MOVED DUE TO SHOWN ELEMENTS, ADD ITS POSITION TO 'PREINTERPOS' ARRAY
    
            $pre.each(function(){
                this.data.preInterPos = $(this).offset();
            });
            
            // SET DISPLAY PROPERTY OF PRE-EXISTING ELEMENTS INCASE WE ARE CHANGING LAYOUT MODE
    
            if(config.layoutMode == 'list'){
                $pre.css('display',config.targetDisplayList);
            } else {
                $pre.css('display',config.targetDisplayGrid);
            };
            
            // IF A SORT ARGUMENT HAS BEEN SENT, RUN SORT FUNCTION SO OBJECTS WILL MOVE TO THEIR FINAL ORDER
            
            if(sortby){
                sort(sortby, order, $cont, config); 
            };
            
            // IF VISIBLE SORT ORDER IS THE SAME (WHICH WOULD NOT TRIGGER A TRANSITION EVENT)
        
            if(sortby && compareArr(config.origSort, config.checkSort)){
                
                // THEN CLEAN UP AND GO HOME
                resetFilter();
                return false;
            };
            
            // TEMPORARILY HIDE ALL SHOWN ELEMENTS TO HIDE

            $tohide.hide();
            
            // FOR EACH ELEMENT TO SHOW, AND NOW MOVED DUE TO HIDDEN ELEMENTS BEING REMOVED, 
            // ADD ITS POSITION TO 'FINALPOS' ARRAY
            
            $toshow.each(function(i){
                this.data.finalPos = $(this).offset();
            });
            
            // FOR EACH PRE-EXISTING ELEMENT NOW MOVED DUE TO HIDDEN ELEMENTS BEING REMOVED,
            // ADD ITS POSITION TO 'FINALPREPOS' ARRAY
    
            $pre.each(function(){
                this.data.finalPrePos = $(this).offset();
            });
            
            // SINCE WE ARE IN OUT FINAL STATE, GET NEW HEIGHT OF CONTAINER
    
            config.newHeight = $par.height();
            
            // IF A SORT ARGUMENT AS BEEN SENT, RUN SORT FUNCTION 'RESET' TO MOVE ELEMENTS BACK TO THEIR STARTING ORDER
            
            if(sortby){
                sort('reset', null, $cont, config);
            };
            
            // RE-HIDE ALL ELEMENTS TEMPORARILY SHOWN
            
            $toshow.hide();
            
            // SET DISPLAY PROPERTY OF PRE-EXISTING ELEMENTS BACK TO THEIR 
            // ORIGINAL PROPERTY, INCASE WE ARE CHANGING LAYOUT MODE
            
            $pre.css('display',config.origDisplay);
            
            // ADD/REMOVE GRID AND LIST CLASSES FROM CONTAINER
    
            if(config.origDisplay == 'block'){
                $cont.addClass(config.listClass);
                $toshow.css('display', config.targetDisplayList);
            } else {
                $cont.removeClass(config.listClass);
                $toshow.css('display', config.targetDisplayGrid);
            };
            
            // IF WE ARE ANIMATING CONTAINER, RESET IT TO ITS STARTING HEIGHT
        
            if(config.resizeContainer)$par.css('height', config.origHeight+'px');
    
            // ADD TRANSFORMS TO ALL ELEMENTS TO SHOW
            
            var toShowCSS = {};
            
            for(var i = 0; i<2; i++){
                var a = i==0 ? a = config.prefix : '';
                toShowCSS[a+'transform'] = config.scale+' '+config.rotateX+' '+config.rotateY+' '+config.rotateZ;
                toShowCSS[a+'filter'] = config.blur+' '+config.grayscale;
            };
            
            $toshow.css(toShowCSS);
    
            // FOR EACH PRE-EXISTING ELEMENT, SUBTRACT ITS INTERMEDIATE POSITION FROM ITS ORIGINAL POSITION 
            // TO GET ITS STARTING OFFSET
    
            $pre.each(function(){
                var data = this.data,
                $t = $(this);
                
                if ($t.hasClass('mix_tohide')){
                    data.preTX = data.origPos.left - data.hideInterPos.left;
                    data.preTY = data.origPos.top - data.hideInterPos.top;
                } else {
                    data.preTX = data.origPos.left - data.preInterPos.left;
                    data.preTY = data.origPos.top - data.preInterPos.top;
                };
                var preCSS = {};
                for(var i = 0; i<2; i++){
                    var a = i==0 ? a = config.prefix : '';
                    preCSS[a+'transform'] = 'translate('+data.preTX+'px,'+data.preTY+'px)';
                };
                
                $t.css(preCSS); 
            });
            
            // ADD/REMOVE GRID AND LIST CLASSES FROM CONTAINER
    
            if(config.layoutMode == 'list'){
                $cont.addClass(config.listClass);
                $cont.removeClass(config.gridClass);
            } else {
                $cont.addClass(config.gridClass);
                $cont.removeClass(config.listClass);
            };
            
            // WRAP ANIMATION FUNCTIONS IN 10ms TIMEOUT TO PREVENT RACE CONDITION
            
            var delay = setTimeout(function(){
        
                // APPLY TRANSITION TIMING TO CONTAINER, AND BEGIN ANIMATION TO NEW HEIGHT
                
                if(config.resizeContainer){
                    var containerCSS = {};
                    for(var i = 0; i<2; i++){
                        var a = i==0 ? a = config.prefix : '';
                        containerCSS[a+'transition'] = 'all '+speed+'ms ease-in-out';
                        containerCSS['height'] = config.newHeight+'px';
                    };
                    $par.css(containerCSS);
                };
    
                // BEGIN FADING IN/OUT OF ALL ELEMENTS TO SHOW/HIDE
                $tohide.css('opacity',config.fade);
                $toshow.css('opacity',1);
    
                // FOR EACH ELEMENT BEING SHOWN, CALCULATE ITS TRAJECTORY BY SUBTRACTING
                // ITS INTERMEDIATE POSITION FROM ITS FINAL POSITION.
                // ALSO ADD SPEED AND EASING
                
                $toshow.each(function(){
                    var data = this.data;
                    data.tX = data.finalPos.left - data.showInterPos.left;
                    data.tY = data.finalPos.top - data.showInterPos.top;
                    
                    var toShowCSS = {};
                    for(var i = 0; i<2; i++){
                        var a = i==0 ? a = config.prefix : '';
                        toShowCSS[a+'transition-property'] = a+'transform, '+a+'filter, opacity';
                        toShowCSS[a+'transition-timing-function'] = config.easing+', linear, linear';
                        toShowCSS[a+'transition-duration'] = speed+'ms';
                        toShowCSS[a+'transition-delay'] = '0';
                        toShowCSS[a+'transform'] = 'translate('+data.tX+'px,'+data.tY+'px)';
                        toShowCSS[a+'filter'] = 'none';
                    };
                    
                    $(this).css('-webkit-transition', 'all '+speed+'ms '+config.easingFallback).css(toShowCSS);
                });
                
                // FOR EACH PRE-EXISTING ELEMENT, IF IT HAS A FINAL POSITION, CALCULATE 
                // ITS TRAJETORY BY SUBTRACTING ITS INTERMEDIATE POSITION FROM ITS FINAL POSITION.
                // ALSO ADD SPEED AND EASING
                
                $pre.each(function(){
                    var data = this.data
                    data.tX = data.finalPrePos.left != 0 ? data.finalPrePos.left - data.preInterPos.left : 0;
                    data.tY = data.finalPrePos.left != 0 ? data.finalPrePos.top - data.preInterPos.top : 0;
                    
                    var preCSS = {};
                    for(var i = 0; i<2; i++){
                        var a = i==0 ? a = config.prefix : '';
                        preCSS[a+'transition'] = 'all '+speed+'ms '+config.easing;
                        preCSS[a+'transform'] = 'translate('+data.tX+'px,'+data.tY+'px)';
                    };
                    
                    $(this).css('-webkit-transition', 'all '+speed+'ms '+config.easingFallback).css(preCSS);
                });
        
                // BEGIN TRANSFORMS ON ALL ELEMENTS TO BE HIDDEN
                
                var toHideCSS = {};
                for(var i = 0; i<2; i++){
                    var a = i==0 ? a = config.prefix : '';
                    toHideCSS[a+'transition'] = 'all '+speed+'ms '+config.easing+', '+a+'filter '+speed+'ms linear, opacity '+speed+'ms linear';
                    toHideCSS[a+'transform'] = config.scale+' '+config.rotateX+' '+config.rotateY+' '+config.rotateZ;
                    toHideCSS[a+'filter'] = config.blur+' '+config.grayscale;
                    toHideCSS['opacity'] = config.fade;
                };
                
                $tohide.css(toHideCSS);
                
                // ALL ANIMATIONS HAVE NOW BEEN STARTED, NOW LISTEN FOR TRANSITION END:
                
                $par.bind('webkitTransitionEnd transitionend otransitionend oTransitionEnd',function(e){
                    
                    if (e.originalEvent.propertyName.indexOf('transform') > -1 || e.originalEvent.propertyName.indexOf('opacity') > -1){
                        
                        if(mixSelector.indexOf('.') > -1){
                        
                        // IF MIXSELECTOR IS A CLASS NAME
                        
                            if($(e.target).hasClass(mixSelector.replace('.',''))){
                                resetFilter();
                            };
                        
                        } else {
                            
                        // IF MIXSELECTOR IS A TAG
                        
                            if($(e.target).is(mixSelector)){
                                resetFilter();
                            };
                            
                        };
                        
                    };
                }); 
    
            },10);
            
            // LAST RESORT EMERGENCY FAILSAFE
            
            config.failsafe = setTimeout(function(){
                if(config.mixing){
                    resetFilter();
                };
            }, speed + 400);
    
        } else {
            
        // ELSE IF NOTHING TO SHOW, AND EVERYTHING TO BE HIDDEN
        
            // IF WE ARE RESIZING CONTAINER, SET ITS STARTING HEIGHT
    
            if(config.resizeContainer)$par.css('height', config.origHeight+'px');
            
            // IF IE, FUCK OFF, AND THEN GO HOME
            
            if(!window.atob){
                resetFilter();
                return false;
            };
            
            // GROUP ALL ELEMENTS TO HIDE INTO JQUERY OBJECT
            
            $tohide = $hide;
            
            // WRAP ANIMATION FUNCTIONS IN A 10ms DELAY TO PREVENT RACE CONDITION
    
            var delay = setTimeout(function(){
                
                // APPLY PERSPECTIVE TO CONTAINER
    
                $par.css(config.perspective);
                
                // APPLY TRANSITION TIMING TO CONTAINER, AND BEGIN ANIMATION TO NEW HEIGHT
        
                if(config.resizeContainer){
                    var containerCSS = {};
                    for(var i = 0; i<2; i++){
                        var a = i==0 ? a = config.prefix : '';
                        containerCSS[a+'transition'] = 'height '+speed+'ms ease-in-out';
                        containerCSS['height'] = config.minHeight+'px';
                    };
                    $par.css(containerCSS);
                };
    
                // APPLY TRANSITION TIMING TO ALL TARGET ELEMENTS
                
                $targets.css(config.transition);
                
                // GET TOTAL NUMBER OF ELEMENTS TO HIDE
    
                var totalHide = $hide.length;
                
                // IF SOMETHING TO HIDE:
    
                if(totalHide){
                    
                    // BEGIN TRANSFORMS ON ALL ELEMENTS TO BE HIDDEN

                    var toHideCSS = {};
                    for(var i = 0; i<2; i++){
                        var a = i==0 ? a = config.prefix : '';
                        toHideCSS[a+'transform'] = config.scale+' '+config.rotateX+' '+config.rotateY+' '+config.rotateZ;
                        toHideCSS[a+'filter'] = config.blur+' '+config.grayscale;
                        toHideCSS['opacity'] = config.fade;
                    };

                    $tohide.css(toHideCSS);
                    
                    // ALL ANIMATIONS HAVE NOW BEEN STARTED, NOW LISTEN FOR TRANSITION END:

                    $par.bind('webkitTransitionEnd transitionend otransitionend oTransitionEnd',function(e){
                        if (e.originalEvent.propertyName.indexOf('transform') > -1 || e.originalEvent.propertyName.indexOf('opacity') > -1){
                            $cont.addClass(config.failClass);
                            resetFilter();
                        };
                    });
        
                } else {
                    
                // ELSE, WE'RE DONE MIXING
                    
                    config.mixing = false;
                };
    
            }, 10);
        }; 
        
        // CLEAN UP AND RESET FUNCTION

        function resetFilter(){
            
            // UNBIND TRANSITION END EVENTS FROM CONTAINER
            
            $par.unbind('webkitTransitionEnd transitionend otransitionend oTransitionEnd');
            
            // IF A SORT ARGUMENT HAS BEEN SENT, SORT ELEMENTS TO THEIR FINAL ORDER
            
            if(sortby){
                sort(sortby, order, $cont, config);
            };
            
            // EMPTY SORTING ARRAYS
        
            config.startOrder = [], config.newOrder = [], config.origSort = [], config.checkSort = [];
        
            // REMOVE INLINE STYLES FROM ALL TARGET ELEMENTS AND SLAM THE BRAKES ON
            
            $targets.removeStyle(
                config.prefix+'filter, filter, '+config.prefix+'transform, transform, opacity, display'
            ).css(config.clean).removeAttr('data-checksum');
            
            // BECAUSE IE SUCKS
            
            if(!window.atob){
                $targets.css({
                    display: 'none',
                    opacity: '0'
                });
            };
            
            // REMOVE HEIGHT FROM CONTAINER ONLY IF RESIZING
            
            var remH = config.resizeContainer ? 'height' : '';
            
            // REMOVE INLINE STYLES FROM CONTAINER
        
            $par.removeStyle(
                config.prefix+'transition, transition, '+config.prefix+'perspective, perspective, '+config.prefix+'perspective-origin, perspective-origin, '+remH
            );
            
            // ADD FINAL DISPLAY PROPERTIES AND OPACITY TO ALL SHOWN ELEMENTS
            // CACHE CURRENT LAYOUT MODE & SORT FOR NEXT MIX
            
            if(config.layoutMode == 'list'){
                $show.css({display:config.targetDisplayList, opacity:'1'});
                config.origDisplay = config.targetDisplayList;
            } else {
                $show.css({display:config.targetDisplayGrid, opacity:'1'});
                config.origDisplay = config.targetDisplayGrid;
            };
            config.origLayout = config.layoutMode;
                
            var wait = setTimeout(function(){
                
                // LET GO OF THE BRAKES
                
                $targets.removeStyle(config.prefix+'transition, transition');
            
                // WE'RE DONE MIXING
            
                config.mixing = false;
            
                // FIRE "ONMIXEND" CALLBACK
            
                if(typeof config.onMixEnd == 'function') {
                    var output = config.onMixEnd.call(this, config);
                
                    // UPDATE CONFIG IF DATA RETURNED
                
                    config = output ? output : config;
                };
            });
        };
    };
    
    // SORT FUNCTION
    
    function sort(sortby, order, $cont, config){

        // COMPARE BY ATTRIBUTE

        function compare(a,b) {
            var sortAttrA = isNaN(a.attr(sortby) * 1) ? a.attr(sortby).toLowerCase() : a.attr(sortby) * 1,
                sortAttrB = isNaN(b.attr(sortby) * 1) ? b.attr(sortby).toLowerCase() : b.attr(sortby) * 1;
            if (sortAttrA < sortAttrB)
                return -1;
            if (sortAttrA > sortAttrB)
                return 1;
            return 0;
        };
        
        // REBUILD DOM

        function rebuild(element){
            if(order == 'asc'){
                $sortWrapper.prepend(element).prepend(' ');
            } else {
                $sortWrapper.append(element).append(' ');
            };
        };
        
        // RANDOMIZE ARRAY

        function arrayShuffle(oldArray){
            var newArray = oldArray.slice();
            var len = newArray.length;
            var i = len;
            while (i--){
                var p = parseInt(Math.random()*len);
                var t = newArray[i];
                newArray[i] = newArray[p];
                newArray[p] = t;
            };
            return newArray; 
        };
        
        // SORT
        
        $cont.find(config.targetSelector).wrapAll('<div class="mix_sorter"/>');
        
        var $sortWrapper = $cont.find('.mix_sorter');
        
        if(!config.origSort.length){
            $sortWrapper.find(config.targetSelector+':visible').each(function(){
                $(this).wrap('<s/>');
                config.origSort.push($(this).parent().html().replace(/\s+/g, ''));
                $(this).unwrap();
            });
        };
        
        
        
        $sortWrapper.empty();
        
        if(sortby == 'reset'){
            $.each(config.startOrder,function(){
                $sortWrapper.append(this).append(' ');
            });
        } else if(sortby == 'default'){
            $.each(config.origOrder,function(){
                rebuild(this);
            });
        } else if(sortby == 'random'){
            if(!config.newOrder.length){
                config.newOrder = arrayShuffle(config.startOrder);
            };
            $.each(config.newOrder,function(){
                $sortWrapper.append(this).append(' ');
            }); 
        } else if(sortby == 'custom'){
            $.each(order, function(){
                rebuild(this);
            });
        } else { 
            // SORT BY ATTRIBUTE
            
            if(typeof config.origOrder[0].attr(sortby) === 'undefined'){
                console.log('No such attribute found. Terminating');
                return false;
            };
            
            if(!config.newOrder.length){
                $.each(config.origOrder,function(){
                    config.newOrder.push($(this));
                });
                config.newOrder.sort(compare);
            };
            $.each(config.newOrder,function(){
                rebuild(this);
            });
            
        };
        config.checkSort = [];
        $sortWrapper.find(config.targetSelector+':visible').each(function(i){
            var $t = $(this);
            if(i == 0){
                
                // PREVENT COMPARE RETURNING FALSE POSITIVES ON ELEMENTS WITH NO CLASS/ATTRIBUTES
                
                $t.attr('data-checksum','1');
            };
            $t.wrap('<s/>');
            config.checkSort.push($t.parent().html().replace(/\s+/g, ''));
            $t.unwrap();
        });
        
        $cont.find(config.targetSelector).unwrap();
    };
    
    // FIND VENDOR PREFIX

    function prefix(el) {
        var prefixes = ["Webkit", "Moz", "O", "ms"];
        for (var i = 0; i < prefixes.length; i++){
            if (prefixes[i] + "Transition" in el.style){
                return prefixes[i];
            };
        };
        return "transition" in el.style ? "" : false;
    };
    
    // REMOVE SPECIFIC STYLES
    
    $.fn.removeStyle = function(style){
        return this.each(function(){
            var obj = $(this);
            style = style.replace(/\s+/g, '');
            var styles = style.split(',');
            $.each(styles,function(){
                
                var search = new RegExp(this.toString() + '[^;]+;?', 'g');
                obj.attr('style', function(i, style){
                    if(style) return style.replace(search, '');
                });
            });
        });
    };

    // COMPARE ARRAYS 
    
    function compareArr(a,b){
        if (a.length != b.length) return false;
        for (var i = 0; i < b.length; i++){
            if (a[i].compare) { 
                if (!a[i].compare(b[i])) return false;
            };
            if (a[i] !== b[i]) return false;
        };
        return true;
    };
    
    // BUILD FILTER ARRAY(S)
    
    function buildFilterArray(str){
        // CLEAN FILTER STRING
        str = str.replace(/\s{2,}/g, ' ');
        // FOR EACH PEROID SEPERATED CLASS NAME, ADD STRING TO FILTER ARRAY
        var arr = str.split(' ');
        // IF ALL, REPLACE WITH MIX_ALL
        $.each(arr,function(i){
            if(this == 'all')arr[i] = 'mix_all';
        });
        if(arr[0] == "")arr.shift(); 
        return arr;
    };

    
})(jQuery);


/*
 *  jQuery OwlCarousel v1.3.3
 *
 *  Copyright (c) 2013 Bartosz Wojciechowski
 *  http://www.owlgraphic.com/owlcarousel/
 *
 *  Licensed under MIT
 *
 */

/*JS Lint helpers: */
/*global dragMove: false, dragEnd: false, $, jQuery, alert, window, document */
/*jslint nomen: true, continue:true */

if (typeof Object.create !== "function") {
    Object.create = function (obj) {
        function F() {}
        F.prototype = obj;
        return new F();
    };
}
(function ($, window, document) {

    var Carousel = {
        init : function (options, el) {
            var base = this;

            base.$elem = $(el);
            base.options = $.extend({}, $.fn.owlCarousel.options, base.$elem.data(), options);

            base.userOptions = options;
            base.loadContent();
        },

        loadContent : function () {
            var base = this, url;

            function getData(data) {
                var i, content = "";
                if (typeof base.options.jsonSuccess === "function") {
                    base.options.jsonSuccess.apply(this, [data]);
                } else {
                    for (i in data.owl) {
                        if (data.owl.hasOwnProperty(i)) {
                            content += data.owl[i].item;
                        }
                    }
                    base.$elem.html(content);
                }
                base.logIn();
            }

            if (typeof base.options.beforeInit === "function") {
                base.options.beforeInit.apply(this, [base.$elem]);
            }

            if (typeof base.options.jsonPath === "string") {
                url = base.options.jsonPath;
                $.getJSON(url, getData);
            } else {
                base.logIn();
            }
        },

        logIn : function () {
            var base = this;

            base.$elem.data("owl-originalStyles", base.$elem.attr("style"));
            base.$elem.data("owl-originalClasses", base.$elem.attr("class"));

            base.$elem.css({opacity: 0});
            base.orignalItems = base.options.items;
            base.checkBrowser();
            base.wrapperWidth = 0;
            base.checkVisible = null;
            base.setVars();
        },

        setVars : function () {
            var base = this;
            if (base.$elem.children().length === 0) {return false; }
            base.baseClass();
            base.eventTypes();
            base.$userItems = base.$elem.children();
            base.itemsAmount = base.$userItems.length;
            base.wrapItems();
            base.$owlItems = base.$elem.find(".owl-item");
            base.$owlWrapper = base.$elem.find(".owl-wrapper");
            base.playDirection = "next";
            base.prevItem = 0;
            base.prevArr = [0];
            base.currentItem = 0;
            base.customEvents();
            base.onStartup();
        },

        onStartup : function () {
            var base = this;
            base.updateItems();
            base.calculateAll();
            base.buildControls();
            base.updateControls();
            base.response();
            base.moveEvents();
            base.stopOnHover();
            base.owlStatus();

            if (base.options.transitionStyle !== false) {
                base.transitionTypes(base.options.transitionStyle);
            }
            if (base.options.autoPlay === true) {
                base.options.autoPlay = 5000;
            }
            base.play();

            base.$elem.find(".owl-wrapper").css("display", "block");

            if (!base.$elem.is(":visible")) {
                base.watchVisibility();
            } else {
                base.$elem.css("opacity", 1);
            }
            base.onstartup = false;
            base.eachMoveUpdate();
            if (typeof base.options.afterInit === "function") {
                base.options.afterInit.apply(this, [base.$elem]);
            }
        },

        eachMoveUpdate : function () {
            var base = this;

            if (base.options.lazyLoad === true) {
                base.lazyLoad();
            }
            if (base.options.autoHeight === true) {
                base.autoHeight();
            }
            base.onVisibleItems();

            if (typeof base.options.afterAction === "function") {
                base.options.afterAction.apply(this, [base.$elem]);
            }
        },

        updateVars : function () {
            var base = this;
            if (typeof base.options.beforeUpdate === "function") {
                base.options.beforeUpdate.apply(this, [base.$elem]);
            }
            base.watchVisibility();
            base.updateItems();
            base.calculateAll();
            base.updatePosition();
            base.updateControls();
            base.eachMoveUpdate();
            if (typeof base.options.afterUpdate === "function") {
                base.options.afterUpdate.apply(this, [base.$elem]);
            }
        },

        reload : function () {
            var base = this;
            window.setTimeout(function () {
                base.updateVars();
            }, 0);
        },

        watchVisibility : function () {
            var base = this;

            if (base.$elem.is(":visible") === false) {
                base.$elem.css({opacity: 0});
                window.clearInterval(base.autoPlayInterval);
                window.clearInterval(base.checkVisible);
            } else {
                return false;
            }
            base.checkVisible = window.setInterval(function () {
                if (base.$elem.is(":visible")) {
                    base.reload();
                    base.$elem.animate({opacity: 1}, 200);
                    window.clearInterval(base.checkVisible);
                }
            }, 500);
        },

        wrapItems : function () {
            var base = this;
            base.$userItems.wrapAll("<div class=\"owl-wrapper\">").wrap("<div class=\"owl-item\"></div>");
            base.$elem.find(".owl-wrapper").wrap("<div class=\"owl-wrapper-outer\">");
            base.wrapperOuter = base.$elem.find(".owl-wrapper-outer");
            base.$elem.css("display", "block");
        },

        baseClass : function () {
            var base = this,
                hasBaseClass = base.$elem.hasClass(base.options.baseClass),
                hasThemeClass = base.$elem.hasClass(base.options.theme);

            if (!hasBaseClass) {
                base.$elem.addClass(base.options.baseClass);
            }

            if (!hasThemeClass) {
                base.$elem.addClass(base.options.theme);
            }
        },

        updateItems : function () {
            var base = this, width, i;

            if (base.options.responsive === false) {
                return false;
            }
            if (base.options.singleItem === true) {
                base.options.items = base.orignalItems = 1;
                base.options.itemsCustom = false;
                base.options.itemsDesktop = false;
                base.options.itemsDesktopSmall = false;
                base.options.itemsTablet = false;
                base.options.itemsTabletSmall = false;
                base.options.itemsMobile = false;
                return false;
            }

            width = $(base.options.responsiveBaseWidth).width();

            if (width > (base.options.itemsDesktop[0] || base.orignalItems)) {
                base.options.items = base.orignalItems;
            }
            if (base.options.itemsCustom !== false) {
                //Reorder array by screen size
                base.options.itemsCustom.sort(function (a, b) {return a[0] - b[0]; });

                for (i = 0; i < base.options.itemsCustom.length; i += 1) {
                    if (base.options.itemsCustom[i][0] <= width) {
                        base.options.items = base.options.itemsCustom[i][1];
                    }
                }

            } else {

                if (width <= base.options.itemsDesktop[0] && base.options.itemsDesktop !== false) {
                    base.options.items = base.options.itemsDesktop[1];
                }

                if (width <= base.options.itemsDesktopSmall[0] && base.options.itemsDesktopSmall !== false) {
                    base.options.items = base.options.itemsDesktopSmall[1];
                }

                if (width <= base.options.itemsTablet[0] && base.options.itemsTablet !== false) {
                    base.options.items = base.options.itemsTablet[1];
                }

                if (width <= base.options.itemsTabletSmall[0] && base.options.itemsTabletSmall !== false) {
                    base.options.items = base.options.itemsTabletSmall[1];
                }

                if (width <= base.options.itemsMobile[0] && base.options.itemsMobile !== false) {
                    base.options.items = base.options.itemsMobile[1];
                }
            }

            //if number of items is less than declared
            if (base.options.items > base.itemsAmount && base.options.itemsScaleUp === true) {
                base.options.items = base.itemsAmount;
            }
        },

        response : function () {
            var base = this,
                smallDelay,
                lastWindowWidth;

            if (base.options.responsive !== true) {
                return false;
            }
            lastWindowWidth = $(window).width();

            base.resizer = function () {
                if ($(window).width() !== lastWindowWidth) {
                    if (base.options.autoPlay !== false) {
                        window.clearInterval(base.autoPlayInterval);
                    }
                    window.clearTimeout(smallDelay);
                    smallDelay = window.setTimeout(function () {
                        lastWindowWidth = $(window).width();
                        base.updateVars();
                    }, base.options.responsiveRefreshRate);
                }
            };
            $(window).resize(base.resizer);
        },

        updatePosition : function () {
            var base = this;
            base.jumpTo(base.currentItem);
            if (base.options.autoPlay !== false) {
                base.checkAp();
            }
        },

        appendItemsSizes : function () {
            var base = this,
                roundPages = 0,
                lastItem = base.itemsAmount - base.options.items;

            base.$owlItems.each(function (index) {
                var $this = $(this);
                $this
                    .css({"width": base.itemWidth})
                    .data("owl-item", Number(index));

                if (index % base.options.items === 0 || index === lastItem) {
                    if (!(index > lastItem)) {
                        roundPages += 1;
                    }
                }
                $this.data("owl-roundPages", roundPages);
            });
        },

        appendWrapperSizes : function () {
            var base = this,
                width = base.$owlItems.length * base.itemWidth;

            base.$owlWrapper.css({
                "width": width * 2,
                "left": 0
            });
            base.appendItemsSizes();
        },

        calculateAll : function () {
            var base = this;
            base.calculateWidth();
            base.appendWrapperSizes();
            base.loops();
            base.max();
        },

        calculateWidth : function () {
            var base = this;
            base.itemWidth = Math.round(base.$elem.width() / base.options.items);
        },

        max : function () {
            var base = this,
                maximum = ((base.itemsAmount * base.itemWidth) - base.options.items * base.itemWidth) * -1;
            if (base.options.items > base.itemsAmount) {
                base.maximumItem = 0;
                maximum = 0;
                base.maximumPixels = 0;
            } else {
                base.maximumItem = base.itemsAmount - base.options.items;
                base.maximumPixels = maximum;
            }
            return maximum;
        },

        min : function () {
            return 0;
        },

        loops : function () {
            var base = this,
                prev = 0,
                elWidth = 0,
                i,
                item,
                roundPageNum;

            base.positionsInArray = [0];
            base.pagesInArray = [];

            for (i = 0; i < base.itemsAmount; i += 1) {
                elWidth += base.itemWidth;
                base.positionsInArray.push(-elWidth);

                if (base.options.scrollPerPage === true) {
                    item = $(base.$owlItems[i]);
                    roundPageNum = item.data("owl-roundPages");
                    if (roundPageNum !== prev) {
                        base.pagesInArray[prev] = base.positionsInArray[i];
                        prev = roundPageNum;
                    }
                }
            }
        },

        buildControls : function () {
            var base = this;
            if (base.options.navigation === true || base.options.pagination === true) {
                base.owlControls = $("<div class=\"owl-controls\"/>").toggleClass("clickable", !base.browser.isTouch).appendTo(base.$elem);
            }
            if (base.options.pagination === true) {
                base.buildPagination();
            }
            if (base.options.navigation === true) {
                base.buildButtons();
            }
        },

        buildButtons : function () {
            var base = this,
                buttonsWrapper = $("<div class=\"owl-buttons\"/>");
            base.owlControls.append(buttonsWrapper);

            base.buttonPrev = $("<div/>", {
                "class" : "owl-prev",
                "html" : base.options.navigationText[0] || ""
            });

            base.buttonNext = $("<div/>", {
                "class" : "owl-next",
                "html" : base.options.navigationText[1] || ""
            });

            buttonsWrapper
                .append(base.buttonPrev)
                .append(base.buttonNext);

            buttonsWrapper.on("touchstart.owlControls mousedown.owlControls", "div[class^=\"owl\"]", function (event) {
                event.preventDefault();
            });

            buttonsWrapper.on("touchend.owlControls mouseup.owlControls", "div[class^=\"owl\"]", function (event) {
                event.preventDefault();
                if ($(this).hasClass("owl-next")) {
                    base.next();
                } else {
                    base.prev();
                }
            });
        },

        buildPagination : function () {
            var base = this;

            base.paginationWrapper = $("<div class=\"owl-pagination\"/>");
            base.owlControls.append(base.paginationWrapper);

            base.paginationWrapper.on("touchend.owlControls mouseup.owlControls", ".owl-page", function (event) {
                event.preventDefault();
                if (Number($(this).data("owl-page")) !== base.currentItem) {
                    base.goTo(Number($(this).data("owl-page")), true);
                }
            });
        },

        updatePagination : function () {
            var base = this,
                counter,
                lastPage,
                lastItem,
                i,
                paginationButton,
                paginationButtonInner;

            if (base.options.pagination === false) {
                return false;
            }

            base.paginationWrapper.html("");

            counter = 0;
            lastPage = base.itemsAmount - base.itemsAmount % base.options.items;

            for (i = 0; i < base.itemsAmount; i += 1) {
                if (i % base.options.items === 0) {
                    counter += 1;
                    if (lastPage === i) {
                        lastItem = base.itemsAmount - base.options.items;
                    }
                    paginationButton = $("<div/>", {
                        "class" : "owl-page"
                    });
                    paginationButtonInner = $("<span></span>", {
                        "text": base.options.paginationNumbers === true ? counter : "",
                        "class": base.options.paginationNumbers === true ? "owl-numbers" : ""
                    });
                    paginationButton.append(paginationButtonInner);

                    paginationButton.data("owl-page", lastPage === i ? lastItem : i);
                    paginationButton.data("owl-roundPages", counter);

                    base.paginationWrapper.append(paginationButton);
                }
            }
            base.checkPagination();
        },
        checkPagination : function () {
            var base = this;
            if (base.options.pagination === false) {
                return false;
            }
            base.paginationWrapper.find(".owl-page").each(function () {
                if ($(this).data("owl-roundPages") === $(base.$owlItems[base.currentItem]).data("owl-roundPages")) {
                    base.paginationWrapper
                        .find(".owl-page")
                        .removeClass("active");
                    $(this).addClass("active");
                }
            });
        },

        checkNavigation : function () {
            var base = this;

            if (base.options.navigation === false) {
                return false;
            }
            if (base.options.rewindNav === false) {
                if (base.currentItem === 0 && base.maximumItem === 0) {
                    base.buttonPrev.addClass("disabled");
                    base.buttonNext.addClass("disabled");
                } else if (base.currentItem === 0 && base.maximumItem !== 0) {
                    base.buttonPrev.addClass("disabled");
                    base.buttonNext.removeClass("disabled");
                } else if (base.currentItem === base.maximumItem) {
                    base.buttonPrev.removeClass("disabled");
                    base.buttonNext.addClass("disabled");
                } else if (base.currentItem !== 0 && base.currentItem !== base.maximumItem) {
                    base.buttonPrev.removeClass("disabled");
                    base.buttonNext.removeClass("disabled");
                }
            }
        },

        updateControls : function () {
            var base = this;
            base.updatePagination();
            base.checkNavigation();
            if (base.owlControls) {
                if (base.options.items >= base.itemsAmount) {
                    base.owlControls.hide();
                } else {
                    base.owlControls.show();
                }
            }
        },

        destroyControls : function () {
            var base = this;
            if (base.owlControls) {
                base.owlControls.remove();
            }
        },

        next : function (speed) {
            var base = this;

            if (base.isTransition) {
                return false;
            }

            base.currentItem += base.options.scrollPerPage === true ? base.options.items : 1;
            if (base.currentItem > base.maximumItem + (base.options.scrollPerPage === true ? (base.options.items - 1) : 0)) {
                if (base.options.rewindNav === true) {
                    base.currentItem = 0;
                    speed = "rewind";
                } else {
                    base.currentItem = base.maximumItem;
                    return false;
                }
            }
            base.goTo(base.currentItem, speed);
        },

        prev : function (speed) {
            var base = this;

            if (base.isTransition) {
                return false;
            }

            if (base.options.scrollPerPage === true && base.currentItem > 0 && base.currentItem < base.options.items) {
                base.currentItem = 0;
            } else {
                base.currentItem -= base.options.scrollPerPage === true ? base.options.items : 1;
            }
            if (base.currentItem < 0) {
                if (base.options.rewindNav === true) {
                    base.currentItem = base.maximumItem;
                    speed = "rewind";
                } else {
                    base.currentItem = 0;
                    return false;
                }
            }
            base.goTo(base.currentItem, speed);
        },

        goTo : function (position, speed, drag) {
            var base = this,
                goToPixel;

            if (base.isTransition) {
                return false;
            }
            if (typeof base.options.beforeMove === "function") {
                base.options.beforeMove.apply(this, [base.$elem]);
            }
            if (position >= base.maximumItem) {
                position = base.maximumItem;
            } else if (position <= 0) {
                position = 0;
            }

            base.currentItem = base.owl.currentItem = position;
            if (base.options.transitionStyle !== false && drag !== "drag" && base.options.items === 1 && base.browser.support3d === true) {
                base.swapSpeed(0);
                if (base.browser.support3d === true) {
                    base.transition3d(base.positionsInArray[position]);
                } else {
                    base.css2slide(base.positionsInArray[position], 1);
                }
                base.afterGo();
                base.singleItemTransition();
                return false;
            }
            goToPixel = base.positionsInArray[position];

            if (base.browser.support3d === true) {
                base.isCss3Finish = false;

                if (speed === true) {
                    base.swapSpeed("paginationSpeed");
                    window.setTimeout(function () {
                        base.isCss3Finish = true;
                    }, base.options.paginationSpeed);

                } else if (speed === "rewind") {
                    base.swapSpeed(base.options.rewindSpeed);
                    window.setTimeout(function () {
                        base.isCss3Finish = true;
                    }, base.options.rewindSpeed);

                } else {
                    base.swapSpeed("slideSpeed");
                    window.setTimeout(function () {
                        base.isCss3Finish = true;
                    }, base.options.slideSpeed);
                }
                base.transition3d(goToPixel);
            } else {
                if (speed === true) {
                    base.css2slide(goToPixel, base.options.paginationSpeed);
                } else if (speed === "rewind") {
                    base.css2slide(goToPixel, base.options.rewindSpeed);
                } else {
                    base.css2slide(goToPixel, base.options.slideSpeed);
                }
            }
            base.afterGo();
        },

        jumpTo : function (position) {
            var base = this;
            if (typeof base.options.beforeMove === "function") {
                base.options.beforeMove.apply(this, [base.$elem]);
            }
            if (position >= base.maximumItem || position === -1) {
                position = base.maximumItem;
            } else if (position <= 0) {
                position = 0;
            }
            base.swapSpeed(0);
            if (base.browser.support3d === true) {
                base.transition3d(base.positionsInArray[position]);
            } else {
                base.css2slide(base.positionsInArray[position], 1);
            }
            base.currentItem = base.owl.currentItem = position;
            base.afterGo();
        },

        afterGo : function () {
            var base = this;

            base.prevArr.push(base.currentItem);
            base.prevItem = base.owl.prevItem = base.prevArr[base.prevArr.length - 2];
            base.prevArr.shift(0);

            if (base.prevItem !== base.currentItem) {
                base.checkPagination();
                base.checkNavigation();
                base.eachMoveUpdate();

                if (base.options.autoPlay !== false) {
                    base.checkAp();
                }
            }
            if (typeof base.options.afterMove === "function" && base.prevItem !== base.currentItem) {
                base.options.afterMove.apply(this, [base.$elem]);
            }
        },

        stop : function () {
            var base = this;
            base.apStatus = "stop";
            window.clearInterval(base.autoPlayInterval);
        },

        checkAp : function () {
            var base = this;
            if (base.apStatus !== "stop") {
                base.play();
            }
        },

        play : function () {
            var base = this;
            base.apStatus = "play";
            if (base.options.autoPlay === false) {
                return false;
            }
            window.clearInterval(base.autoPlayInterval);
            base.autoPlayInterval = window.setInterval(function () {
                base.next(true);
            }, base.options.autoPlay);
        },

        swapSpeed : function (action) {
            var base = this;
            if (action === "slideSpeed") {
                base.$owlWrapper.css(base.addCssSpeed(base.options.slideSpeed));
            } else if (action === "paginationSpeed") {
                base.$owlWrapper.css(base.addCssSpeed(base.options.paginationSpeed));
            } else if (typeof action !== "string") {
                base.$owlWrapper.css(base.addCssSpeed(action));
            }
        },

        addCssSpeed : function (speed) {
            return {
                "-webkit-transition": "all " + speed + "ms ease",
                "-moz-transition": "all " + speed + "ms ease",
                "-o-transition": "all " + speed + "ms ease",
                "transition": "all " + speed + "ms ease"
            };
        },

        removeTransition : function () {
            return {
                "-webkit-transition": "",
                "-moz-transition": "",
                "-o-transition": "",
                "transition": ""
            };
        },

        doTranslate : function (pixels) {
            return {
                "-webkit-transform": "translate3d(" + pixels + "px, 0px, 0px)",
                "-moz-transform": "translate3d(" + pixels + "px, 0px, 0px)",
                "-o-transform": "translate3d(" + pixels + "px, 0px, 0px)",
                "-ms-transform": "translate3d(" + pixels + "px, 0px, 0px)",
                "transform": "translate3d(" + pixels + "px, 0px,0px)"
            };
        },

        transition3d : function (value) {
            var base = this;
            base.$owlWrapper.css(base.doTranslate(value));
        },

        css2move : function (value) {
            var base = this;
            base.$owlWrapper.css({"left" : value});
        },

        css2slide : function (value, speed) {
            var base = this;

            base.isCssFinish = false;
            base.$owlWrapper.stop(true, true).animate({
                "left" : value
            }, {
                duration : speed || base.options.slideSpeed,
                complete : function () {
                    base.isCssFinish = true;
                }
            });
        },

        checkBrowser : function () {
            var base = this,
                translate3D = "translate3d(0px, 0px, 0px)",
                tempElem = document.createElement("div"),
                regex,
                asSupport,
                support3d,
                isTouch;

            tempElem.style.cssText = "  -moz-transform:" + translate3D +
                                  "; -ms-transform:"     + translate3D +
                                  "; -o-transform:"      + translate3D +
                                  "; -webkit-transform:" + translate3D +
                                  "; transform:"         + translate3D;
            regex = /translate3d\(0px, 0px, 0px\)/g;
            asSupport = tempElem.style.cssText.match(regex);
            support3d = (asSupport !== null && asSupport.length === 1);

            isTouch = "ontouchstart" in window || window.navigator.msMaxTouchPoints;

            base.browser = {
                "support3d" : support3d,
                "isTouch" : isTouch
            };
        },

        moveEvents : function () {
            var base = this;
            if (base.options.mouseDrag !== false || base.options.touchDrag !== false) {
                base.gestures();
                base.disabledEvents();
            }
        },

        eventTypes : function () {
            var base = this,
                types = ["s", "e", "x"];

            base.ev_types = {};

            if (base.options.mouseDrag === true && base.options.touchDrag === true) {
                types = [
                    "touchstart.owl mousedown.owl",
                    "touchmove.owl mousemove.owl",
                    "touchend.owl touchcancel.owl mouseup.owl"
                ];
            } else if (base.options.mouseDrag === false && base.options.touchDrag === true) {
                types = [
                    "touchstart.owl",
                    "touchmove.owl",
                    "touchend.owl touchcancel.owl"
                ];
            } else if (base.options.mouseDrag === true && base.options.touchDrag === false) {
                types = [
                    "mousedown.owl",
                    "mousemove.owl",
                    "mouseup.owl"
                ];
            }

            base.ev_types.start = types[0];
            base.ev_types.move = types[1];
            base.ev_types.end = types[2];
        },

        disabledEvents :  function () {
            var base = this;
            base.$elem.on("dragstart.owl", function (event) { event.preventDefault(); });
            base.$elem.on("mousedown.disableTextSelect", function (e) {
                return $(e.target).is('input, textarea, select, option');
            });
        },

        gestures : function () {
            /*jslint unparam: true*/
            var base = this,
                locals = {
                    offsetX : 0,
                    offsetY : 0,
                    baseElWidth : 0,
                    relativePos : 0,
                    position: null,
                    minSwipe : null,
                    maxSwipe: null,
                    sliding : null,
                    dargging: null,
                    targetElement : null
                };

            base.isCssFinish = true;

            function getTouches(event) {
                if (event.touches !== undefined) {
                    return {
                        x : event.touches[0].pageX,
                        y : event.touches[0].pageY
                    };
                }

                if (event.touches === undefined) {
                    if (event.pageX !== undefined) {
                        return {
                            x : event.pageX,
                            y : event.pageY
                        };
                    }
                    if (event.pageX === undefined) {
                        return {
                            x : event.clientX,
                            y : event.clientY
                        };
                    }
                }
            }

            function swapEvents(type) {
                if (type === "on") {
                    $(document).on(base.ev_types.move, dragMove);
                    $(document).on(base.ev_types.end, dragEnd);
                } else if (type === "off") {
                    $(document).off(base.ev_types.move);
                    $(document).off(base.ev_types.end);
                }
            }

            function dragStart(event) {
                var ev = event.originalEvent || event || window.event,
                    position;

                if (ev.which === 3) {
                    return false;
                }
                if (base.itemsAmount <= base.options.items) {
                    return;
                }
                if (base.isCssFinish === false && !base.options.dragBeforeAnimFinish) {
                    return false;
                }
                if (base.isCss3Finish === false && !base.options.dragBeforeAnimFinish) {
                    return false;
                }

                if (base.options.autoPlay !== false) {
                    window.clearInterval(base.autoPlayInterval);
                }

                if (base.browser.isTouch !== true && !base.$owlWrapper.hasClass("grabbing")) {
                    base.$owlWrapper.addClass("grabbing");
                }

                base.newPosX = 0;
                base.newRelativeX = 0;

                $(this).css(base.removeTransition());

                position = $(this).position();
                locals.relativePos = position.left;

                locals.offsetX = getTouches(ev).x - position.left;
                locals.offsetY = getTouches(ev).y - position.top;

                swapEvents("on");

                locals.sliding = false;
                locals.targetElement = ev.target || ev.srcElement;
            }

            function dragMove(event) {
                var ev = event.originalEvent || event || window.event,
                    minSwipe,
                    maxSwipe;

                base.newPosX = getTouches(ev).x - locals.offsetX;
                base.newPosY = getTouches(ev).y - locals.offsetY;
                base.newRelativeX = base.newPosX - locals.relativePos;

                if (typeof base.options.startDragging === "function" && locals.dragging !== true && base.newRelativeX !== 0) {
                    locals.dragging = true;
                    base.options.startDragging.apply(base, [base.$elem]);
                }

                if ((base.newRelativeX > 8 || base.newRelativeX < -8) && (base.browser.isTouch === true)) {
                    if (ev.preventDefault !== undefined) {
                        ev.preventDefault();
                    } else {
                        ev.returnValue = false;
                    }
                    locals.sliding = true;
                }

                if ((base.newPosY > 10 || base.newPosY < -10) && locals.sliding === false) {
                    $(document).off("touchmove.owl");
                }

                minSwipe = function () {
                    return base.newRelativeX / 5;
                };

                maxSwipe = function () {
                    return base.maximumPixels + base.newRelativeX / 5;
                };

                base.newPosX = Math.max(Math.min(base.newPosX, minSwipe()), maxSwipe());
                if (base.browser.support3d === true) {
                    base.transition3d(base.newPosX);
                } else {
                    base.css2move(base.newPosX);
                }
            }

            function dragEnd(event) {
                var ev = event.originalEvent || event || window.event,
                    newPosition,
                    handlers,
                    owlStopEvent;

                ev.target = ev.target || ev.srcElement;

                locals.dragging = false;

                if (base.browser.isTouch !== true) {
                    base.$owlWrapper.removeClass("grabbing");
                }

                if (base.newRelativeX < 0) {
                    base.dragDirection = base.owl.dragDirection = "left";
                } else {
                    base.dragDirection = base.owl.dragDirection = "right";
                }

                if (base.newRelativeX !== 0) {
                    newPosition = base.getNewPosition();
                    base.goTo(newPosition, false, "drag");
                    if (locals.targetElement === ev.target && base.browser.isTouch !== true) {
                        $(ev.target).on("click.disable", function (ev) {
                            ev.stopImmediatePropagation();
                            ev.stopPropagation();
                            ev.preventDefault();
                            $(ev.target).off("click.disable");
                        });
                        handlers = $._data(ev.target, "events").click;
                        owlStopEvent = handlers.pop();
                        handlers.splice(0, 0, owlStopEvent);
                    }
                }
                swapEvents("off");
            }
            base.$elem.on(base.ev_types.start, ".owl-wrapper", dragStart);
        },

        getNewPosition : function () {
            var base = this,
                newPosition = base.closestItem();

            if (newPosition > base.maximumItem) {
                base.currentItem = base.maximumItem;
                newPosition  = base.maximumItem;
            } else if (base.newPosX >= 0) {
                newPosition = 0;
                base.currentItem = 0;
            }
            return newPosition;
        },
        closestItem : function () {
            var base = this,
                array = base.options.scrollPerPage === true ? base.pagesInArray : base.positionsInArray,
                goal = base.newPosX,
                closest = null;

            $.each(array, function (i, v) {
                if (goal - (base.itemWidth / 20) > array[i + 1] && goal - (base.itemWidth / 20) < v && base.moveDirection() === "left") {
                    closest = v;
                    if (base.options.scrollPerPage === true) {
                        base.currentItem = $.inArray(closest, base.positionsInArray);
                    } else {
                        base.currentItem = i;
                    }
                } else if (goal + (base.itemWidth / 20) < v && goal + (base.itemWidth / 20) > (array[i + 1] || array[i] - base.itemWidth) && base.moveDirection() === "right") {
                    if (base.options.scrollPerPage === true) {
                        closest = array[i + 1] || array[array.length - 1];
                        base.currentItem = $.inArray(closest, base.positionsInArray);
                    } else {
                        closest = array[i + 1];
                        base.currentItem = i + 1;
                    }
                }
            });
            return base.currentItem;
        },

        moveDirection : function () {
            var base = this,
                direction;
            if (base.newRelativeX < 0) {
                direction = "right";
                base.playDirection = "next";
            } else {
                direction = "left";
                base.playDirection = "prev";
            }
            return direction;
        },

        customEvents : function () {
            /*jslint unparam: true*/
            var base = this;
            base.$elem.on("owl.next", function () {
                base.next();
            });
            base.$elem.on("owl.prev", function () {
                base.prev();
            });
            base.$elem.on("owl.play", function (event, speed) {
                base.options.autoPlay = speed;
                base.play();
                base.hoverStatus = "play";
            });
            base.$elem.on("owl.stop", function () {
                base.stop();
                base.hoverStatus = "stop";
            });
            base.$elem.on("owl.goTo", function (event, item) {
                base.goTo(item);
            });
            base.$elem.on("owl.jumpTo", function (event, item) {
                base.jumpTo(item);
            });
        },

        stopOnHover : function () {
            var base = this;
            if (base.options.stopOnHover === true && base.browser.isTouch !== true && base.options.autoPlay !== false) {
                base.$elem.on("mouseover", function () {
                    base.stop();
                });
                base.$elem.on("mouseout", function () {
                    if (base.hoverStatus !== "stop") {
                        base.play();
                    }
                });
            }
        },

        lazyLoad : function () {
            var base = this,
                i,
                $item,
                itemNumber,
                $lazyImg,
                follow;

            if (base.options.lazyLoad === false) {
                return false;
            }
            for (i = 0; i < base.itemsAmount; i += 1) {
                $item = $(base.$owlItems[i]);

                if ($item.data("owl-loaded") === "loaded") {
                    continue;
                }

                itemNumber = $item.data("owl-item");
                $lazyImg = $item.find(".lazyOwl");

                if (typeof $lazyImg.data("src") !== "string") {
                    $item.data("owl-loaded", "loaded");
                    continue;
                }
                if ($item.data("owl-loaded") === undefined) {
                    $lazyImg.hide();
                    $item.addClass("loading").data("owl-loaded", "checked");
                }
                if (base.options.lazyFollow === true) {
                    follow = itemNumber >= base.currentItem;
                } else {
                    follow = true;
                }
                if (follow && itemNumber < base.currentItem + base.options.items && $lazyImg.length) {
                    base.lazyPreload($item, $lazyImg);
                }
            }
        },

        lazyPreload : function ($item, $lazyImg) {
            var base = this,
                iterations = 0,
                isBackgroundImg;

            if ($lazyImg.prop("tagName") === "DIV") {
                $lazyImg.css("background-image", "url(" + $lazyImg.data("src") + ")");
                isBackgroundImg = true;
            } else {
                $lazyImg[0].src = $lazyImg.data("src");
            }

            function showImage() {
                $item.data("owl-loaded", "loaded").removeClass("loading");
                $lazyImg.removeAttr("data-src");
                if (base.options.lazyEffect === "fade") {
                    $lazyImg.fadeIn(400);
                } else {
                    $lazyImg.show();
                }
                if (typeof base.options.afterLazyLoad === "function") {
                    base.options.afterLazyLoad.apply(this, [base.$elem]);
                }
            }

            function checkLazyImage() {
                iterations += 1;
                if (base.completeImg($lazyImg.get(0)) || isBackgroundImg === true) {
                    showImage();
                } else if (iterations <= 100) {//if image loads in less than 10 seconds 
                    window.setTimeout(checkLazyImage, 100);
                } else {
                    showImage();
                }
            }

            checkLazyImage();
        },

        autoHeight : function () {
            var base = this,
                $currentimg = $(base.$owlItems[base.currentItem]).find("img"),
                iterations;

            function addHeight() {
                var $currentItem = $(base.$owlItems[base.currentItem]).height();
                base.wrapperOuter.css("height", $currentItem + "px");
                if (!base.wrapperOuter.hasClass("autoHeight")) {
                    window.setTimeout(function () {
                        base.wrapperOuter.addClass("autoHeight");
                    }, 0);
                }
            }

            function checkImage() {
                iterations += 1;
                if (base.completeImg($currentimg.get(0))) {
                    addHeight();
                } else if (iterations <= 100) { //if image loads in less than 10 seconds 
                    window.setTimeout(checkImage, 100);
                } else {
                    base.wrapperOuter.css("height", ""); //Else remove height attribute
                }
            }

            if ($currentimg.get(0) !== undefined) {
                iterations = 0;
                checkImage();
            } else {
                addHeight();
            }
        },

        completeImg : function (img) {
            var naturalWidthType;

            if (!img.complete) {
                return false;
            }
            naturalWidthType = typeof img.naturalWidth;
            if (naturalWidthType !== "undefined" && img.naturalWidth === 0) {
                return false;
            }
            return true;
        },

        onVisibleItems : function () {
            var base = this,
                i;

            if (base.options.addClassActive === true) {
                base.$owlItems.removeClass("active");
            }
            base.visibleItems = [];
            for (i = base.currentItem; i < base.currentItem + base.options.items; i += 1) {
                base.visibleItems.push(i);

                if (base.options.addClassActive === true) {
                    $(base.$owlItems[i]).addClass("active");
                }
            }
            base.owl.visibleItems = base.visibleItems;
        },

        transitionTypes : function (className) {
            var base = this;
            //Currently available: "fade", "backSlide", "goDown", "fadeUp"
            base.outClass = "owl-" + className + "-out";
            base.inClass = "owl-" + className + "-in";
        },

        singleItemTransition : function () {
            var base = this,
                outClass = base.outClass,
                inClass = base.inClass,
                $currentItem = base.$owlItems.eq(base.currentItem),
                $prevItem = base.$owlItems.eq(base.prevItem),
                prevPos = Math.abs(base.positionsInArray[base.currentItem]) + base.positionsInArray[base.prevItem],
                origin = Math.abs(base.positionsInArray[base.currentItem]) + base.itemWidth / 2,
                animEnd = 'webkitAnimationEnd oAnimationEnd MSAnimationEnd animationend';

            base.isTransition = true;

            base.$owlWrapper
                .addClass('owl-origin')
                .css({
                    "-webkit-transform-origin" : origin + "px",
                    "-moz-perspective-origin" : origin + "px",
                    "perspective-origin" : origin + "px"
                });
            function transStyles(prevPos) {
                return {
                    "position" : "relative",
                    "left" : prevPos + "px"
                };
            }

            $prevItem
                .css(transStyles(prevPos, 10))
                .addClass(outClass)
                .on(animEnd, function () {
                    base.endPrev = true;
                    $prevItem.off(animEnd);
                    base.clearTransStyle($prevItem, outClass);
                });

            $currentItem
                .addClass(inClass)
                .on(animEnd, function () {
                    base.endCurrent = true;
                    $currentItem.off(animEnd);
                    base.clearTransStyle($currentItem, inClass);
                });
        },

        clearTransStyle : function (item, classToRemove) {
            var base = this;
            item.css({
                "position" : "",
                "left" : ""
            }).removeClass(classToRemove);

            if (base.endPrev && base.endCurrent) {
                base.$owlWrapper.removeClass('owl-origin');
                base.endPrev = false;
                base.endCurrent = false;
                base.isTransition = false;
            }
        },

        owlStatus : function () {
            var base = this;
            base.owl = {
                "userOptions"   : base.userOptions,
                "baseElement"   : base.$elem,
                "userItems"     : base.$userItems,
                "owlItems"      : base.$owlItems,
                "currentItem"   : base.currentItem,
                "prevItem"      : base.prevItem,
                "visibleItems"  : base.visibleItems,
                "isTouch"       : base.browser.isTouch,
                "browser"       : base.browser,
                "dragDirection" : base.dragDirection
            };
        },

        clearEvents : function () {
            var base = this;
            base.$elem.off(".owl owl mousedown.disableTextSelect");
            $(document).off(".owl owl");
            $(window).off("resize", base.resizer);
        },

        unWrap : function () {
            var base = this;
            if (base.$elem.children().length !== 0) {
                base.$owlWrapper.unwrap();
                base.$userItems.unwrap().unwrap();
                if (base.owlControls) {
                    base.owlControls.remove();
                }
            }
            base.clearEvents();
            base.$elem
                .attr("style", base.$elem.data("owl-originalStyles") || "")
                .attr("class", base.$elem.data("owl-originalClasses"));
        },

        destroy : function () {
            var base = this;
            base.stop();
            window.clearInterval(base.checkVisible);
            base.unWrap();
            base.$elem.removeData();
        },

        reinit : function (newOptions) {
            var base = this,
                options = $.extend({}, base.userOptions, newOptions);
            base.unWrap();
            base.init(options, base.$elem);
        },

        addItem : function (htmlString, targetPosition) {
            var base = this,
                position;

            if (!htmlString) {return false; }

            if (base.$elem.children().length === 0) {
                base.$elem.append(htmlString);
                base.setVars();
                return false;
            }
            base.unWrap();
            if (targetPosition === undefined || targetPosition === -1) {
                position = -1;
            } else {
                position = targetPosition;
            }
            if (position >= base.$userItems.length || position === -1) {
                base.$userItems.eq(-1).after(htmlString);
            } else {
                base.$userItems.eq(position).before(htmlString);
            }

            base.setVars();
        },

        removeItem : function (targetPosition) {
            var base = this,
                position;

            if (base.$elem.children().length === 0) {
                return false;
            }
            if (targetPosition === undefined || targetPosition === -1) {
                position = -1;
            } else {
                position = targetPosition;
            }

            base.unWrap();
            base.$userItems.eq(position).remove();
            base.setVars();
        }

    };

    $.fn.owlCarousel = function (options) {
        return this.each(function () {
            if ($(this).data("owl-init") === true) {
                return false;
            }
            $(this).data("owl-init", true);
            var carousel = Object.create(Carousel);
            carousel.init(options, this);
            $.data(this, "owlCarousel", carousel);
        });
    };

    $.fn.owlCarousel.options = {

        items : 5,
        itemsCustom : false,
        itemsDesktop : [1199, 4],
        itemsDesktopSmall : [979, 3],
        itemsTablet : [768, 2],
        itemsTabletSmall : false,
        itemsMobile : [479, 1],
        singleItem : false,
        itemsScaleUp : false,

        slideSpeed : 200,
        paginationSpeed : 800,
        rewindSpeed : 1000,

        autoPlay : false,
        stopOnHover : false,

        navigation : false,
        navigationText : ["prev", "next"],
        rewindNav : true,
        scrollPerPage : false,

        pagination : true,
        paginationNumbers : false,

        responsive : true,
        responsiveRefreshRate : 200,
        responsiveBaseWidth : window,

        baseClass : "owl-carousel",
        theme : "owl-theme",

        lazyLoad : false,
        lazyFollow : true,
        lazyEffect : "fade",

        autoHeight : false,

        jsonPath : false,
        jsonSuccess : false,

        dragBeforeAnimFinish : true,
        mouseDrag : true,
        touchDrag : true,

        addClassActive : false,
        transitionStyle : false,

        beforeUpdate : false,
        afterUpdate : false,
        beforeInit : false,
        afterInit : false,
        beforeMove : false,
        afterMove : false,
        afterAction : false,
        startDragging : false,
        afterLazyLoad: false
    };
}(jQuery, window, document));

