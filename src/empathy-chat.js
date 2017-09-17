/*
 * Copyright (C) 2012 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Debarshi Ray <debarshir@src.gnome.org>
 */


var chat = document.getElementById("Chat");


function createHTMLNode(html) {
  var range = document.createRange();
  range.selectNode(chat);
  return range.createContextualFragment(html);
}


function getContentsNode(node) {
  var post = node.getElementsByClassName("post")[0];
  return post.getElementsByClassName("post-contents")[0];
}


// Remove all but the last #insert
function removeInsertNodes(node) {
  var inserts = node.querySelectorAll("#insert");
  if (inserts.length < 2)
    return;

  for (var i = 0; i < inserts.length - 1; i++) {
    var insert = inserts[i];
    insert.parentNode.removeChild(insert);
  }
}


// Remove all but the first #prepend
function removePrependNodes(node) {
  var pres = node.querySelectorAll("#prepend");
  if (pres.length < 2)
    return;

  for (var i = 1; i < pres.length; i++) {
    var pre = pres[i];
    pre.parentNode.removeChild(pre);
  }
}


function prepend(html) {
  var node = createHTMLNode(html);
  chat.insertBefore(node, chat.firstChild);

  // The lastChild should retain the #insert
  if (chat.children.length == 1)
    return;

  removeInsertNodes(chat);
  removePrependNodes(chat);
}


function prependPrev(html) {
  var pre = chat.firstChild.querySelector("#prepend");

  // For themes that don't support #prepend
  if (!pre) {
    prepend(html);
    return;
  }

  var contents = pre.parentNode;
  var node = createHTMLNode(html);

  // Skip both the #prepend and #insert
  for (var i = node.childNodes.length - 2; i > 0; i--)
    contents.insertBefore(node.childNodes[i], pre.nextSibling);
}
