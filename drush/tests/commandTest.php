<?php

class commandCase extends Drush_CommandTestCase {
  public function testInvoke() {
    $expected = array(
      'unit_drush_init',
      'drush_unit_invoke_init',
      'drush_unit_invoke_validate',
      'drush_unit_pre_unit_invoke',
      'drush_unit_invoke',
      'drush_unit_post_unit_invoke',
      'drush_unit_post_unit_invoke_rollback',
      'drush_unit_pre_unit_invoke_rollback',
      'drush_unit_invoke_validate_rollback',
    );

    // We expect a return code of 1 so just call execute() directly.
    $exec = sprintf('%s unit-invoke --include=%s --nocolor', UNISH_DRUSH, self::escapeshellarg(dirname(__FILE__)));
    $this->execute($exec, self::EXIT_ERROR);
    $called = json_decode($this->getOutput());
    $this->assertSame($expected, $called);
  }

  /*
   * Assert that minimum bootstrap phase is honored.
   *
   * Not testing dependency on a module since that requires an installed Drupal.
   * Too slow for little benefit.
   */
  public function testRequirementBootstrapPhase() {
    // Assure that core-cron fails when run outside of a Drupal site.
    $return = $this->execute(UNISH_DRUSH . ' core-cron --quiet --nocolor', self::EXIT_ERROR);
  }

  /**
   * Assert that unknown options are caught and flagged as errors
   */
  public function testUnknownOptions() {
    // Make sure an ordinary 'version' command works
    $return = $this->drush('version', array(), array('pipe', 'strict'));
    // Add an unknown option --magic=1234 and insure it fails
    $return = $this->execute(UNISH_DRUSH . ' version --pipe --magic=1234 --strict --nocolor', self::EXIT_ERROR);
    // Finally, add in a hook that uses drush_hook_help_alter to allow the 'magic' option.
    // We need to run 'drush cc drush' to clear the commandfile cache; otherwise, our include will not be found.
    $include_path = dirname(__FILE__) . '/hooks/magic_help_alter';
    $this->drush('version', array(), array('include' => $include_path, 'pipe' => NULL, 'magic' => '1234', 'strict' => NULL));
  }
}
