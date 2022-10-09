/* Copyright 2021 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

/* This file is generated from:
 *  colors_test.json5
 *  colors_test_palette.json5
 *  typography_test.json5
 */

import {css} from 'lit';
/* SAFETY_BOILERPLATE */

export interface GetColorsCSSOptions {
  /** The html selector the colors should be attached too. */
  target?: 'host' | 'html';
  /**
   * Generate a css dump which sets variables to their dark mode values in light
   * mode and vice versa. If true lockTheme is ignored.
   */
  invert?: boolean;
  /**
   * Generate a css dump which sets variables to either their dark mode or light
   * mode values and ignores the documents prefers-color-scheme.
   */
  lockTheme?: 'light' | 'dark';
}

// Use a ternary expression that can only be evaluated at runtime here to force
// closure to leave these string constants as variables instead of inlining them
// in the below template strings. Not doing this results in a unreasonable file
// size increase. See b/209520919.
const DEFAULT_CSS = window ? `
  --google-grey-900-rgb: 32, 33, 36;
  --google-grey-900: rgb(var(--google-grey-900-rgb));

  --cros-text-color-primary-rgb: var(--google-grey-900-rgb);
  --cros-text-color-primary: var(--google-grey-900);

  --cros-toggle-color-rgb: var(--cros-text-color-primary-rgb);
  --cros-toggle-color: rgba(var(--cros-toggle-color-rgb), 0.1);

  --cros-bg-color-elevation-1-rgb: 255, 255, 255;
  --cros-bg-color-elevation-1: rgb(var(--cros-bg-color-elevation-1-rgb));

  --cros-disabled-opacity: 0.38;

  --cros-reference-opacity: var(--cros-disabled-opacity);
` : '';

const DARK_MODE_OVERRIDES_CSS = window ? `
  --cros-text-color-primary-rgb: 255, 255, 255;
  --cros-text-color-primary: rgb(var(--cros-text-color-primary-rgb));

  --cros-toggle-color-rgb: var(--cros-text-color-primary-rgb);
  --cros-toggle-color: rgba(var(--cros-toggle-color-rgb), var(--cros-disabled-opacity));

  --cros-bg-color-elevation-1-rgb: 41, 42, 45;
  --cros-bg-color-elevation-1: rgb(var(--cros-bg-color-elevation-1-rgb));

  --cros-reference-opacity: 1;
` : '';

const UNTYPED_CSS = window ? `` : '';

const TYPOGRAPHY_CSS = window ? `
  /* font families */
  --cros-font-family-test: 'Google Sans', 'Noto Sans', sans-serif;
  --cros-font-family-other: Roboto, 'Noto Sans', sans-serif;

  /* typefaces */
  --cros-headline-1-font: 500 15px/22px var(--cros-font-family-test);
  --cros-headline-1-font-family: var(--cros-font-family-test);
  --cros-headline-1-font-size: 15px;
  --cros-headline-1-font-weight: 500;
  --cros-headline-1-line-height: 22px;
` : '';

/**
 * Returns a string containing all semantic colors exported in this file as
 * css variables. This string an be used to construct a stylesheet which can be
 * placed in the dom at runtime, see tools/style_variable_generator/README.md
 * for more info. Ensure the css returned by this function is added to the dom
 * before the rendering of the first element on the page so you are guaranteed
 * that all TS constant references resolve correctly.
 */
export function getColorsCSS(options?: GetColorsCSSOptions) {
  let cssString;
  if (options?.invert) {
    cssString = /* SAFE */ (`
      ${options?.target === 'host' ? ':host' : 'html:not(body)'} {
        ${DEFAULT_CSS}
        ${UNTYPED_CSS}
        ${TYPOGRAPHY_CSS}
        ${DARK_MODE_OVERRIDES_CSS}
      }

      @media (prefers-color-scheme: dark) {
        ${options?.target === 'host' ? ':host' : 'html:not(body)'} {
          ${DEFAULT_CSS}
        }
      }
    `);
  } else if (options?.lockTheme === 'light') {
    // Tag strings which are safe with a special comment so copybara can add
    // the right safety wrappers whem moving this code into Google3.
    cssString = /* SAFE */ (`
      ${options?.target === 'host' ? ':host' : 'html:not(body)'} {
        ${DEFAULT_CSS}
        ${UNTYPED_CSS}
        ${TYPOGRAPHY_CSS}
      }
    `);
  } else if (options?.lockTheme === 'dark') {
    cssString = /* SAFE */ (`
      ${options?.target === 'host' ? ':host' : 'html:not(body)'} {
        ${DEFAULT_CSS}
        ${UNTYPED_CSS}
        ${TYPOGRAPHY_CSS}
        ${DARK_MODE_OVERRIDES_CSS}
      }
    `);
  } else {
    cssString = /* SAFE */ (`
      ${options?.target === 'host' ? ':host' : 'html:not(body)'} {
        ${DEFAULT_CSS}
        ${UNTYPED_CSS}
        ${TYPOGRAPHY_CSS}
      }
      @media (prefers-color-scheme: dark) {
        ${options?.target === 'host' ? ':host' : 'html:not(body)'} {
          ${DARK_MODE_OVERRIDES_CSS}
        }
      }
    `);
  }

  return cssString;
}

export const GOOGLE_GREY_900 = css`var(--google-grey-900)`;
export const TEXT_COLOR_PRIMARY = css`var(--cros-text-color-primary)`;
export const TOGGLE_COLOR = css`var(--cros-toggle-color)`;
export const BG_COLOR_ELEVATION_1 = css`var(--cros-bg-color-elevation-1)`;
export const DISABLED_OPACITY = css`var(--cros-disabled-opacity)`;
export const REFERENCE_OPACITY = css`var(--cros-reference-opacity)`;

export const FONT_FAMILY_TEST = css`var(--cros-font-family-test-font)`;
export const FONT_FAMILY_OTHER = css`var(--cros-font-family-other-font)`;
export const HEADLINE_1_FONT = css`var(--cros-headline-1-font)`;
export const HEADLINE_1_FONT_FAMILY = css`var(--cros-headline-1-font-family)`;
export const HEADLINE_1_FONT_SIZE = css`var(--cros-headline-1-font-size)`;
export const HEADLINE_1_FONT_WEIGHT = css`var(--cros-headline-1-font-weight)`;
export const HEADLINE_1_LINE_HEIGHT = css`var(--cros-headline-1-line-height)`;
