import test from 'node:test';
import assert from 'node:assert/strict';
import { PANEL_HOST, createPanelLink, readPanelLink } from '../web/panel-link.js';
import { invitationUrl, parseInvitation } from '../companion/panels/respyra/remote-profile.js';

test('embedded controller and observer invitations stay on the owning VLC+ site', () => {
  assert.equal(PANEL_HOST, 'https://georgefejer91.github.io/vlc-plus/remote/');
  const panel = { id: 'fixture', name: 'Fixture', url: 'https://experiment.example/' };
  assert.deepEqual(JSON.parse(readPanelLink(createPanelLink(panel))), panel);
  const invitation = { room: `brsp_${'a'.repeat(64)}`, secret: 'b'.repeat(64) };
  const url = invitationUrl(invitation);
  assert.equal(new URL(url).pathname, '/vlc-plus/remote/panels/respyra/');
  assert.deepEqual(parseInvitation(url), invitation);
  assert.equal(parseInvitation(url.replace('/vlc-plus/remote/', '/other/')), null);
});
