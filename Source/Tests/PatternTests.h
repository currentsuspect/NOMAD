#pragma once

#include <JuceHeader.h>
#include "../Models/Pattern.h"

/**
 * Unit tests for Pattern class
 * Tests note addition, removal, range queries, and copy/paste operations
 */
class PatternTests : public juce::UnitTest
{
public:
    PatternTests() : juce::UnitTest("Pattern Tests", "Models") {}
    
    void runTest() override
    {
        testNoteAddition();
        testNoteRemoval();
        testRangeQueries();
        testPatternCopyPaste();
        testPatternLength();
        testNoteValidation();
    }
    
private:
    void testNoteAddition()
    {
        beginTest("Note Addition");
        
        Pattern pattern("Test Pattern", 16, 4);
        
        // Test adding a single note
        Pattern::Note note1(0, 0, 60, 1.0f, 1);
        pattern.addNote(note1);
        
        auto notes = pattern.getAllNotes();
        expect(notes.size() == 1, "Should have 1 note after adding");
        expect(notes[0] == note1, "Note should match the added note");
        
        // Test adding multiple notes
        Pattern::Note note2(4, 0, 64, 0.8f, 2);
        Pattern::Note note3(8, 1, 67, 0.9f, 1);
        pattern.addNote(note2);
        pattern.addNote(note3);
        
        notes = pattern.getAllNotes();
        expect(notes.size() == 3, "Should have 3 notes after adding");
        
        // Test updating existing note (same step, track, pitch)
        Pattern::Note note1Updated(0, 0, 60, 0.5f, 2);
        pattern.addNote(note1Updated);
        
        notes = pattern.getAllNotes();
        expect(notes.size() == 3, "Should still have 3 notes after update");
        
        auto updatedNote = std::find_if(notes.begin(), notes.end(),
            [](const Pattern::Note& n) { return n.step == 0 && n.track == 0 && n.pitch == 60; });
        
        expect(updatedNote != notes.end(), "Updated note should exist");
        expect(updatedNote->velocity == 0.5f, "Velocity should be updated");
        expect(updatedNote->duration == 2, "Duration should be updated");
    }
    
    void testNoteRemoval()
    {
        beginTest("Note Removal");
        
        Pattern pattern("Test Pattern", 16, 4);
        
        // Add test notes
        pattern.addNote(Pattern::Note(0, 0, 60, 1.0f, 1));
        pattern.addNote(Pattern::Note(0, 0, 64, 0.8f, 1));
        pattern.addNote(Pattern::Note(4, 0, 67, 0.9f, 1));
        pattern.addNote(Pattern::Note(4, 1, 72, 0.7f, 1));
        
        expect(pattern.getAllNotes().size() == 4, "Should have 4 notes initially");
        
        // Test removing by step and track (removes all notes at that position)
        pattern.removeNote(0, 0);
        expect(pattern.getAllNotes().size() == 2, "Should have 2 notes after removing step 0, track 0");
        
        // Test removing by step, track, and pitch (specific note)
        pattern.addNote(Pattern::Note(8, 0, 60, 1.0f, 1));
        pattern.addNote(Pattern::Note(8, 0, 64, 0.8f, 1));
        expect(pattern.getAllNotes().size() == 4, "Should have 4 notes after adding");
        
        pattern.removeNote(8, 0, 60);
        auto notes = pattern.getAllNotes();
        expect(notes.size() == 3, "Should have 3 notes after removing specific note");
        
        // Verify the correct note was removed
        auto found = std::find_if(notes.begin(), notes.end(),
            [](const Pattern::Note& n) { return n.step == 8 && n.track == 0 && n.pitch == 60; });
        expect(found == notes.end(), "Removed note should not exist");
        
        found = std::find_if(notes.begin(), notes.end(),
            [](const Pattern::Note& n) { return n.step == 8 && n.track == 0 && n.pitch == 64; });
        expect(found != notes.end(), "Other note at same position should still exist");
        
        // Test clear all notes
        pattern.clearAllNotes();
        expect(pattern.getAllNotes().size() == 0, "Should have 0 notes after clearing");
    }
    
    void testRangeQueries()
    {
        beginTest("Range Queries");
        
        Pattern pattern("Test Pattern", 16, 4);
        
        // Add notes at various positions
        pattern.addNote(Pattern::Note(0, 0, 60, 1.0f, 1));
        pattern.addNote(Pattern::Note(2, 0, 64, 0.8f, 1));
        pattern.addNote(Pattern::Note(4, 0, 67, 0.9f, 1));
        pattern.addNote(Pattern::Note(8, 0, 72, 0.7f, 1));
        pattern.addNote(Pattern::Note(12, 0, 76, 0.6f, 1));
        
        // Test range query [0, 4) - should get notes at steps 0 and 2
        auto range1 = pattern.getNotesInRange(0, 4);
        expect(range1.size() == 2, "Range [0, 4) should contain 2 notes");
        
        // Test range query [4, 12) - should get notes at steps 4 and 8
        auto range2 = pattern.getNotesInRange(4, 12);
        expect(range2.size() == 2, "Range [4, 12) should contain 2 notes");
        
        // Test range query [0, 16) - should get all notes
        auto range3 = pattern.getNotesInRange(0, 16);
        expect(range3.size() == 5, "Range [0, 16) should contain all 5 notes");
        
        // Test empty range
        auto range4 = pattern.getNotesInRange(5, 7);
        expect(range4.size() == 0, "Range [5, 7) should be empty");
        
        // Test single step range
        auto range5 = pattern.getNotesInRange(4, 5);
        expect(range5.size() == 1, "Range [4, 5) should contain 1 note");
        expect(range5[0].step == 4, "Note should be at step 4");
    }
    
    void testPatternCopyPaste()
    {
        beginTest("Pattern Copy/Paste (Deep Copying)");
        
        Pattern original("Original Pattern", 16, 4);
        
        // Add notes to original
        original.addNote(Pattern::Note(0, 0, 60, 1.0f, 1));
        original.addNote(Pattern::Note(4, 0, 64, 0.8f, 2));
        original.addNote(Pattern::Note(8, 1, 67, 0.9f, 1));
        
        // Test clone method
        auto cloned = original.clone();
        
        expect(cloned != nullptr, "Clone should not be null");
        expect(cloned->getName() == "Original Pattern (Copy)", "Clone should have modified name");
        expect(cloned->getLength() == 16, "Clone should have same length");
        expect(cloned->getStepsPerBeat() == 4, "Clone should have same steps per beat");
        expect(cloned->getAllNotes().size() == 3, "Clone should have same number of notes");
        
        // Verify deep copy - modify original and check clone is unaffected
        original.addNote(Pattern::Note(12, 0, 72, 0.7f, 1));
        expect(original.getAllNotes().size() == 4, "Original should have 4 notes");
        expect(cloned->getAllNotes().size() == 3, "Clone should still have 3 notes (deep copy)");
        
        // Test copyFrom method
        Pattern destination("Destination Pattern", 8, 2);
        destination.addNote(Pattern::Note(0, 0, 48, 0.5f, 1));
        
        destination.copyFrom(original);
        
        expect(destination.getName() == "Original Pattern", "Destination should have original's name");
        expect(destination.getLength() == 16, "Destination should have original's length");
        expect(destination.getStepsPerBeat() == 4, "Destination should have original's steps per beat");
        expect(destination.getAllNotes().size() == 4, "Destination should have original's notes");
        
        // Verify deep copy with copyFrom
        original.clearAllNotes();
        expect(original.getAllNotes().size() == 0, "Original should have 0 notes");
        expect(destination.getAllNotes().size() == 4, "Destination should still have 4 notes (deep copy)");
        
        // Verify note data integrity
        auto destNotes = destination.getAllNotes();
        auto note60 = std::find_if(destNotes.begin(), destNotes.end(),
            [](const Pattern::Note& n) { return n.pitch == 60; });
        expect(note60 != destNotes.end(), "Note with pitch 60 should exist");
        expect(note60->step == 0, "Note should be at step 0");
        expect(note60->velocity == 1.0f, "Note should have velocity 1.0");
    }
    
    void testPatternLength()
    {
        beginTest("Pattern Length Management");
        
        Pattern pattern("Test Pattern", 16, 4);
        
        // Add notes throughout the pattern
        pattern.addNote(Pattern::Note(0, 0, 60, 1.0f, 1));
        pattern.addNote(Pattern::Note(8, 0, 64, 0.8f, 1));
        pattern.addNote(Pattern::Note(15, 0, 67, 0.9f, 1));
        
        expect(pattern.getAllNotes().size() == 3, "Should have 3 notes");
        
        // Reduce pattern length - should remove notes beyond new length
        pattern.setLength(12);
        expect(pattern.getLength() == 12, "Length should be 12");
        
        auto notes = pattern.getAllNotes();
        expect(notes.size() == 2, "Should have 2 notes after reducing length");
        
        // Verify note at step 15 was removed
        auto found = std::find_if(notes.begin(), notes.end(),
            [](const Pattern::Note& n) { return n.step == 15; });
        expect(found == notes.end(), "Note at step 15 should be removed");
        
        // Increase pattern length - existing notes should remain
        pattern.setLength(32);
        expect(pattern.getLength() == 32, "Length should be 32");
        expect(pattern.getAllNotes().size() == 2, "Should still have 2 notes");
    }
    
    void testNoteValidation()
    {
        beginTest("Note Validation");
        
        Pattern pattern("Test Pattern", 16, 4);
        
        // Test invalid step (negative)
        pattern.addNote(Pattern::Note(-1, 0, 60, 1.0f, 1));
        expect(pattern.getAllNotes().size() == 0, "Should not add note with negative step");
        
        // Test invalid step (beyond pattern length)
        pattern.addNote(Pattern::Note(16, 0, 60, 1.0f, 1));
        expect(pattern.getAllNotes().size() == 0, "Should not add note beyond pattern length");
        
        // Test invalid pitch (negative)
        pattern.addNote(Pattern::Note(0, 0, -1, 1.0f, 1));
        expect(pattern.getAllNotes().size() == 0, "Should not add note with negative pitch");
        
        // Test invalid pitch (> 127)
        pattern.addNote(Pattern::Note(0, 0, 128, 1.0f, 1));
        expect(pattern.getAllNotes().size() == 0, "Should not add note with pitch > 127");
        
        // Test invalid velocity (negative)
        pattern.addNote(Pattern::Note(0, 0, 60, -0.1f, 1));
        expect(pattern.getAllNotes().size() == 0, "Should not add note with negative velocity");
        
        // Test invalid velocity (> 1.0)
        pattern.addNote(Pattern::Note(0, 0, 60, 1.1f, 1));
        expect(pattern.getAllNotes().size() == 0, "Should not add note with velocity > 1.0");
        
        // Test valid note
        pattern.addNote(Pattern::Note(0, 0, 60, 1.0f, 1));
        expect(pattern.getAllNotes().size() == 1, "Should add valid note");
    }
};

static PatternTests patternTests;
