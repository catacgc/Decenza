## ADDED Requirements

### Requirement: Session-Only Brew Overrides
The system SHALL provide session-only (non-persisted) overrides for brew parameters: temperature, dose weight, yield weight, and grind setting. These overrides SHALL NOT modify the saved profile and SHALL be cleared after each shot ends or when the user switches profiles.

#### Scenario: User sets temperature override
- **WHEN** the user sets a brew temperature different from the profile default in the BrewDialog
- **THEN** the system stores a session-only temperature override
- **AND** the IdlePage displays the override as an arrow (e.g., "88 → 90°C")
- **AND** the DE1 machine uses the overridden temperature for the next shot

#### Scenario: User sets dose and yield overrides
- **WHEN** the user configures dose and yield in the BrewDialog and confirms
- **THEN** the system activates brew-by-ratio mode with the specified dose and calculated ratio
- **AND** the shot plan line on IdlePage shows the configured dose and yield

#### Scenario: Overrides persist after a shot
- **WHEN** a shot ends (espresso cycle completes)

#### Scenario: Target weight and temperature overrides cleared on profile switch
- **WHEN** the user loads a different profile
- **THEN** the target weight and temperature overrides are cleared

#### Scenario: Grind and dose overrides cleared on bean switch
- **WHEN** the user loads a different bean
- **THEN** the grind setting and dose overrides are cleared
- **THEN** the grind setting is the one configured from the bean
- **THEN** the dose is the default 18g

### Requirement: Brew Dialog
The system SHALL provide a BrewDialog accessible from the shot plan line on IdlePage and from the StatusBar. The dialog SHALL display the current profile name and bean info as a "Base Recipe" header, and allow editing temperature, dose, grind, ratio, and yield for the next shot.

#### Scenario: Opening the BrewDialog
- **WHEN** the user taps the shot plan text on IdlePage
- **THEN** the BrewDialog opens with current values populated from Settings (DYE metadata and profile defaults)
- **AND** the targetWeight and targetTemperature are set with a precedence order: overrides first, then profile defaults

#### Scenario: Temperature override with save option
- **WHEN** the user changes the temperature in the BrewDialog
- **THEN** the dialog shows the profile default temperature below the input
- **AND** a "Save" button allows permanently updating the profile temperature

#### Scenario: Dose from scale
- **WHEN** the user taps "Get from scale" in the BrewDialog
- **AND** the scale reports weight ≥ 3g
- **THEN** the dose value is updated to the scale reading
- **WHEN** the scale reports weight < 3g
- **THEN** a warning is shown asking the user to place the portafilter on the scale

#### Scenario: Ratio and yield auto-calculation
- **WHEN** the user changes dose or ratio
- **THEN** yield is recalculated automatically (dose × ratio)
- **WHEN** the user manually edits the yield value
- **THEN** the ratio is changed automatically (yield / dose)

#### Scenario: Clear all overrides
- **WHEN** the user taps the "Clear" button in the BrewDialog
- **THEN** all fields reset to profile defaults (temperature) and empty/default values (dose=18g, grind=bean"", ratio=calculated from profiel target weight / 18g)

### Requirement: Shot Plan Display
The system SHALL display a summary line on the IdlePage showing the configured shot parameters: profile name with temperature, bean name with grind setting, and dose/yield weights. The line SHALL be clickable to open the BrewDialog.

#### Scenario: Shot plan with no overrides
- **WHEN** no overrides are active and DYE metadata is populated
- **THEN** the shot plan shows: "ProfileName (88°C) · BeanName (grind) · 18.0g in, 36.0g out"

#### Scenario: Shot plan with temperature override
- **WHEN** a temperature override is active
- **THEN** the temperature portion shows the arrow notation: "ProfileName (88 → 90°C)"

#### Scenario: Shot plan hidden when empty
- **WHEN** no profile is loaded and no DYE metadata is set
- **THEN** the shot plan line is not visible

### Requirement: Brew Overrides History Recording
The system SHALL record the active brew overrides (temperature, dose, yield, grind) as a JSON string in the shot history when a shot is saved. This enables traceability of per-shot adjustments.

#### Scenario: Overrides saved to shot history
- **WHEN** a shot ends with active brew overrides
- **THEN** the overrides are serialized to JSON and stored in the `brew_overrides_json` column
- **AND** the overrides JSON is available when viewing the shot in history

#### Scenario: No overrides recorded when none active
- **WHEN** a shot ends without any active brew overrides
- **THEN** the `brew_overrides_json` field is empty/null

### Requirement: Shot History Parameter Retrieval
The system SHALL populate brew parameters (dose, yield, grind) from shot history when a shot is loaded via `loadShotWithMetadata()`. This allows the user to repeat a previous shot's settings.

#### Scenario: Loading shot parameters from history
- **WHEN** the user loads a shot from history
- **THEN** the DYE bean weight is set to the shot's recorded dose
- **AND** the grinder setting is populated from the shot metadata

#### Scenario: BrewDialog pre-populated from history
- **WHEN** the BrewDialog opens after loading a shot from history
- **THEN** dose, yield, and grind fields reflect the loaded shot's parameters
- **AND** the ratio is calculated from the loaded dose and yield
