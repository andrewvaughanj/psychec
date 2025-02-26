// Copyright (c) 2021 Leandro T. C. Melo <ltcmelo@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Parser__IMPL__.inc"

using namespace psy;
using namespace C;

//--------------//
// Declarations //
//--------------//

/**
 * Parse a \a translation-unit.
 */
void Parser::parseTranslationUnit(TranslationUnitSyntax*& unit)
{
    DEBUG_THIS_RULE();

    DeclarationListSyntax** declList_cur = &unit->decls_;

    while (true) {
        DeclarationSyntax* decl = nullptr;
        switch (peek().kind()) {
            case EndOfFile:
                return;

            case Keyword_ExtGNU___extension__: {
                auto extKwTkIdx = consume();
                if (!parseExternalDeclaration(decl))
                    break;;
                PSYCHE_ASSERT(decl, break, "invalid declaration");
                decl->extKwTkIdx_ = extKwTkIdx;
                break;
            }

            default:
                if (parseExternalDeclaration(decl))
                    break;
                ignoreDeclarationOrDefinition();
                continue;
        }

        *declList_cur = makeNode<DeclarationListSyntax>(decl);
        declList_cur = &(*declList_cur)->next;
    }
}

/**
 * Parse an \a external-declaration.
 *
 \verbatim
 external-declaration:
     function-definition
     declaration
 \endverbatim
 *
 * \remark 6.9
 */
bool Parser::parseExternalDeclaration(DeclarationSyntax*& decl)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case SemicolonToken:
            parseIncompleteDeclaration_AtFirst(decl);
            break;

        case Keyword__Static_assert:
            return parseStaticAssertDeclaration_AtFirst(decl);

        case Keyword_ExtGNU___asm__:
            return parseExtGNU_AsmStatementDeclaration_AtFirst(decl);

        case Keyword_ExtPSY__Template:
            parseExtPSY_TemplateDeclaration_AtFirst(decl);
            break;

        default:
            return parseDeclarationOrFunctionDefinition(decl);
    }

    return true;
}

void Parser::parseIncompleteDeclaration_AtFirst(DeclarationSyntax*& decl,
                                                const SpecifierListSyntax* specList)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == SemicolonToken,
                  return,
                  "assert failure: `;'");

    auto incompDecl = makeNode<IncompleteDeclarationSyntax>();
    decl = incompDecl;
    incompDecl->specs_ = specList;
    incompDecl->semicolonTkIdx_ = consume();
}

/**
 * Parse a \a static_assert-declaration, with LA(1) at FIRST.
 *
 \verbatim
 static_assert-declaration:
     _Static_assert ( constant-expression , string-literal ) ;
 \endverbatim
 *
 * \remark 6.7.10
 */
bool Parser::parseStaticAssertDeclaration_AtFirst(DeclarationSyntax*& decl)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword__Static_assert,
                  return false,
                  "assert failure: `_Static_assert'");

    auto assertDecl = makeNode<StaticAssertDeclarationSyntax>();
    decl = assertDecl;
    assertDecl->staticAssertKwTkIdx_ = consume();

    if (match(OpenParenToken, &assertDecl->openParenTkIdx_)
            && parseExpressionWithPrecedenceConditional(assertDecl->expr_)
            && match(CommaToken, &assertDecl->commaTkIdx_)
            && parseStringLiteral(assertDecl->strLit_)
            && match(CloseParenToken, &assertDecl->closeParenTkIdx_)
            && match(SemicolonToken, &assertDecl->semicolonTkIdx_))
        return true;

    skipTo(ColonToken);
    return false;
}

/**
 * Parse a GNU extension file-scope assembly \a statement as a \a declaration,
 * with LA(1) at FIRST.
 */
bool Parser::parseExtGNU_AsmStatementDeclaration_AtFirst(DeclarationSyntax*& decl)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtGNU___asm__,
                  return false,
                  "assert failure: `asm'");

    if (!tree_->options().extensions().isEnabled_ExtGNU_Asm())
        diagnosticsReporter_.ExpectedFeature("GNU assembly in C");

    auto asmDecl = makeNode<ExtGNU_AsmStatementDeclarationSyntax>();
    decl = asmDecl;
    asmDecl->asmTkIdx_ = consume();

    if (match(OpenParenToken, &asmDecl->openParenTkIdx_)
            && parseStringLiteral(asmDecl->strLit_)
            && match(CloseParenToken, &asmDecl->closeParenTkIdx_))
        return true;

    skipTo(CloseParenToken);
    return false;
}

bool Parser::parseDeclaration(
        DeclarationSyntax*& decl,
        bool (Parser::*parseSpecifiers)(DeclarationSyntax*&, SpecifierListSyntax*&, bool),
        bool (Parser::*parse_AtFollowOfSpecifiers)(DeclarationSyntax*&, const SpecifierListSyntax*),
        DeclarationScope declScope)
{
    SpecifierListSyntax* specList = nullptr;
    if (!((this)->*(parseSpecifiers))(
                decl,
                specList,
                declScope != DeclarationScope::Block))
        return false;

    if (peek().kind() == SemicolonToken) {
        if (decl) {
            auto tyDecl = static_cast<TypeDeclarationSyntax*>(decl);
            tyDecl->semicolonTkIdx_ = consume();
        }
        else
            parseIncompleteDeclaration_AtFirst(decl, specList);
        return true;
    }

    if (decl) {
        auto tyDeclSpec = makeNode<TypeDeclarationAsSpecifierSyntax>();
        tyDeclSpec->typeDecl_ = static_cast<TypeDeclarationSyntax*>(decl);
        decl = nullptr;

        if (!specList)
            specList = makeNode<SpecifierListSyntax>(tyDeclSpec);
        else {
            for (auto iter = specList; iter; iter = iter->next) {
                if (iter->value->asTaggedTypeSpecifier()
                        && iter->value == tyDeclSpec->typeDecl_->typeSpec_) {
                    iter->value = tyDeclSpec;
                    break;
                }
            }
        }
    }

    if (!specList) {
        if (declScope == DeclarationScope::File)
            diagnosticsReporter_.ExpectedTypeSpecifier();
        else if (declScope == DeclarationScope::Block)
            diagnosticsReporter_.ExpectedFIRSTofSpecifierQualifier();
    }

    return ((this)->*(parse_AtFollowOfSpecifiers))(decl, specList);
}

/**
 * Parse a (specifier-prefixed) \a declaration or a \a function-definition.
 *
 \verbatim
 declaration:
     declaration-specifiers init-decltor-list_opt ;
     static_assert-declaration

 function-definition:
     declaration-specifiers declarator declaration-list_opt compound-statement
 \endverbatim
 *
 * \remark 6.7, 6.9.1
 */
bool Parser::parseDeclarationOrFunctionDefinition(DeclarationSyntax*& decl)
{
    return parseDeclaration(
                decl,
                &Parser::parseDeclarationSpecifiers,
                &Parser::parseDeclarationOrFunctionDefinition_AtFollowOfSpecifiers,
                DeclarationScope::File);
}

bool Parser::parseDeclarationOrFunctionDefinition_AtFollowOfSpecifiers(
        DeclarationSyntax*& decl,
        const SpecifierListSyntax* specList)
{
    DeclaratorListSyntax* decltorList = nullptr;
    DeclaratorListSyntax** decltorList_cur = &decltorList;

    while (true) {
        DeclaratorSyntax* decltor = nullptr;
        if (!parseDeclarator(decltor, DeclarationScope::File))
            return false;

        *decltorList_cur = makeNode<DeclaratorListSyntax>(decltor);

        InitializerSyntax** init = nullptr;
        if (peek().kind() == EqualsToken) {
            DeclaratorSyntax* stripDecltor = const_cast<DeclaratorSyntax*>(
                    SyntaxUtilities::strippedDeclarator(decltor));
            switch (stripDecltor->kind()) {
                case IdentifierDeclarator: {
                    auto identDecltor = stripDecltor->asIdentifierDeclarator();
                    identDecltor->equalsTkIdx_ = consume();
                    init = &identDecltor->init_;
                    break;
                }

                case PointerDeclarator: {
                    auto ptrDecltor = stripDecltor->asPointerDeclarator();
                    ptrDecltor->equalsTkIdx_ = consume();
                    init = &ptrDecltor->init_;
                    break;
                }

                case ArrayDeclarator: {
                    auto arrDecltor = stripDecltor->asArrayOrFunctionDeclarator();
                    arrDecltor->equalsTkIdx_ = consume();
                    init = &arrDecltor->init_;
                    break;
                }

                case FunctionDeclarator: {
                    auto funcDecltor = stripDecltor->asArrayOrFunctionDeclarator();
                    if (funcDecltor->innerDecltor_) {
                        auto stripInnerDecltor = const_cast<DeclaratorSyntax*>(
                                    SyntaxUtilities::strippedDeclarator(funcDecltor->innerDecltor_));
                        if (stripInnerDecltor->kind() == PointerDeclarator) {
                            funcDecltor->equalsTkIdx_ = consume();
                            init = &funcDecltor->init_;
                            break;
                        }
                    }
                    [[fallthrough]];
                }

                default:
                    diagnosticsReporter_.UnexpectedInitializerOfDeclarator();
                    return ignoreDeclarator();
            }
            if (!parseInitializer(*init))
                return false;
        }

        switch (peek().kind()) {
            case CommaToken:
                (*decltorList_cur)->delimTkIdx_ = consume();
                break;

            case SemicolonToken: {
                auto nameDecl = makeNode<VariableAndOrFunctionDeclarationSyntax>();
                decl = nameDecl;
                nameDecl->semicolonTkIdx_ = consume();
                nameDecl->specs_ = const_cast<SpecifierListSyntax*>(specList);
                nameDecl->decltors_ = decltorList;
                return true;
            }

            case OpenBraceToken:
                if (decltorList_cur == &decltorList) {
                    const DeclaratorSyntax* outerDecltor =
                            SyntaxUtilities::strippedDeclarator(decltor);
                    const DeclaratorSyntax* prevDecltor = nullptr;
                    while (outerDecltor) {
                        const DeclaratorSyntax* innerDecltor =
                                SyntaxUtilities::innerDeclarator(outerDecltor);
                        if (innerDecltor == outerDecltor)
                            break;
                        prevDecltor = outerDecltor;
                        outerDecltor = SyntaxUtilities::strippedDeclarator(innerDecltor);
                    }

                    if (prevDecltor
                            && prevDecltor->kind() == FunctionDeclarator
                            && outerDecltor->kind() == IdentifierDeclarator) {
                        auto funcDef = makeNode<FunctionDefinitionSyntax>();
                        decl = funcDef;
                        funcDef->specs_ = const_cast<SpecifierListSyntax*>(specList);
                        funcDef->decltor_ = decltor;
                        parseCompoundStatement_AtFirst(funcDef->body_, StatementContext::None);
                        return true;
                    }
                }
                [[fallthrough]];

            default:
                if (init)
                    diagnosticsReporter_.ExpectedFOLLOWofInitializedDeclarator();
                else
                    diagnosticsReporter_.ExpectedFOLLOWofDeclarator();
                return false;
        }

        decltorList_cur = &(*decltorList_cur)->next;
    }
}

Parser::IdentifierRole Parser::determineIdentifierRole(bool seenType) const
{
    /*
     Upon an identifier, when parsing a declaration, we can't
     tell whether the identifier is <typedef-name> or a
     <declarator>. Only a "type seen" flag isn't enough to
     help distinguishing the meaning of such identifier: e.g.,
     in `x;', where `x' doesn't name a type, `x' is the
     <declarator> of a variable implicitly typed with `int'
     (a warning is diagnosed, though), but in `x;', given a
     previous `typedef int x;', `x' is a <typedef-name> of an
     empty declaration (again, a warning is diagnosed).

     The way out of this situation, yet allowing valid code
     to be parsed correctly, without an error, is to look
     further ahead for another identifier: if one is found,
     and a <type-specifier> has already been (potentially)
     seen, the found identifier must be a <declarator>;
     however, if a <type-sepcifier> hasn't yet been seen, or
     if an additional identifier wasn't found, then we base
     the decision on other tokens that might be valid within
     a declartor.
     */

    auto parenCnt = 0;
    auto LA = 2;
    while (true) {
        switch (peek(LA).kind()) {
            case IdentifierToken:
                if (seenType)
                    return IdentifierRole::AsDeclarator;

                if (!parenCnt)
                    return IdentifierRole::AsTypedefName;
                seenType = true;
                ++LA;
                continue;

            // type-specifier
            case Keyword_void:
            case Keyword_char:
            case Keyword_short:
            case Keyword_int:
            case Keyword_long:
            case Keyword_float:
            case Keyword_double:
            case Keyword_signed:
            case Keyword_unsigned:
            case Keyword_Ext_char16_t:
            case Keyword_Ext_char32_t:
            case Keyword_Ext_wchar_t:
            case Keyword__Bool:
            case Keyword__Complex:
            case Keyword_struct:
            case Keyword_union:
            case Keyword_enum:
                if (seenType)
                    return IdentifierRole::AsDeclarator;
                seenType = true;
                ++LA;
                continue;

            // storage-class-specifier
            case Keyword_typedef:
            case Keyword_extern:
            case Keyword_static:
            case Keyword_auto:
            case Keyword_register:
            case Keyword__Thread_local:
            case Keyword_ExtGNU___thread:
                ++LA;
                continue;

            // type-qualifier
            case Keyword_const:
            case Keyword_volatile:
            case Keyword_restrict:
            case Keyword__Atomic:
                ++LA;
                continue;

            // function-specifier
            case Keyword_inline:
            case Keyword__Noreturn:
                ++LA;
                continue;

            // alignment-specifier
            case Keyword__Alignas:
                ++LA;
                continue;

            // attribute-specifier
            case Keyword_ExtGNU___attribute__:
                if (!parenCnt)
                    return IdentifierRole::AsTypedefName;
                ++LA;
                continue;

            // pointer-declarator
            case AsteriskToken:
                ++LA;
                continue;

            case OpenParenToken:
                ++parenCnt;
                ++LA;
                continue;

            case CloseParenToken:
                --parenCnt;
                if (!parenCnt) {
                    if (seenType)
                        return IdentifierRole::AsTypedefName;
                    return IdentifierRole::AsDeclarator;
                }
                ++LA;
                continue;

            default:
                return IdentifierRole::AsDeclarator;
        }
    }
}

bool Parser::parseStructDeclaration_AtFollowOfSpecifierQualifierList(
        DeclarationSyntax*& decl,
        const SpecifierListSyntax* specList)
{
    DeclaratorListSyntax* decltorList = nullptr;
    DeclaratorListSyntax** decltorList_cur = &decltorList;

    while (true) {
        DeclaratorSyntax* decltor = nullptr;
        if (!parseDeclarator(decltor, DeclarationScope::Block))
            return false;

        *decltorList_cur = makeNode<DeclaratorListSyntax>(decltor);

        switch (peek().kind()) {
            case CommaToken:
                (*decltorList_cur)->delimTkIdx_ = consume();
                break;

            case SemicolonToken: {
                auto memberDecl = makeNode<FieldDeclarationSyntax>();
                decl = memberDecl;
                memberDecl->semicolonTkIdx_ = consume();
                memberDecl->specs_ = const_cast<SpecifierListSyntax*>(specList);
                memberDecl->decltors_ = decltorList;
                return true;
            }

            default:
                diagnosticsReporter_.ExpectedFOLLOWofDeclarator();
                return false;
        }

        decltorList_cur = &(*decltorList_cur)->next;
    }
}

/**
 * Parse a \a struct-declaration.
 *
 \verbatim
 struct-declaration:
     specifier-qualifier-list struct-declarator-list_opt ;
     static_assert-declaration
 \endverbatim
 *
 * \remark 6.7.2.1
 */
bool Parser::parseStructDeclaration(DeclarationSyntax*& decl)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case Keyword__Static_assert:
            return parseStaticAssertDeclaration_AtFirst(decl);

        case Keyword_ExtGNU___extension__: {
            auto extKwTkIdx = consume();
            if (!parseDeclaration(
                        decl,
                        &Parser::parseSpecifierQualifierList,
                        &Parser::parseStructDeclaration_AtFollowOfSpecifierQualifierList,
                        DeclarationScope::Block))
                return false;
            PSYCHE_ASSERT(decl, return false, "invalid declaration");
            decl->extKwTkIdx_ = extKwTkIdx;
            return true;
        }

        default:
            return parseDeclaration(
                        decl,
                        &Parser::parseSpecifierQualifierList,
                        &Parser::parseStructDeclaration_AtFollowOfSpecifierQualifierList,
                        DeclarationScope::Block);
    }
}

/**
 * Parse an \a enumerator.
 *
 \verbatim
 enumerator:
     enumeration-constant
     enumeration-constant = constant-expression
 \endverbatim
 *
 * \remark 6.7.2.2
 */
bool Parser::parseEnumerator(DeclarationSyntax*& decl)
{
    DEBUG_THIS_RULE();

    EnumMemberDeclarationSyntax* enumMembDecl = nullptr;

    switch (peek().kind()) {
        case IdentifierToken: {
            enumMembDecl = makeNode<EnumMemberDeclarationSyntax>();
            decl = enumMembDecl;
            enumMembDecl->identTkIdx_ = consume();
            break;
        }

        default:
            diagnosticsReporter_.ExpectedFIRSTofEnumerationConstant();
            return false;
    }

    if (peek().kind() == Keyword_ExtGNU___attribute__)
        parseExtGNU_AttributeSpecifierList_AtFirst(enumMembDecl->attrs_);

    switch (peek().kind()) {
        case EqualsToken:
            enumMembDecl->equalsTkIdx_ = consume();
            if (!parseExpressionWithPrecedenceConditional(enumMembDecl->expr_))
                return false;
            if (peek().kind() != CommaToken)
                break;
            [[fallthrough]];

        case CommaToken:
            enumMembDecl->commaTkIdx_ = consume();
            [[fallthrough]];

        default:
            break;
    }

    return true;
}

/**
 * Parse a \a parameter-type-list; or, informally, a "parameter-declaration-
 * list-and-or-ellipsis".
 *
 \verbatim
 parameter-type-list:
     parameter-list
     parameter-list , ...
 \endverbatim
 *
 * \remark 6.7.6
 */
bool Parser::parseParameterDeclarationListAndOrEllipsis(ParameterSuffixSyntax*& paramDecltorSfx)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case CloseParenToken:
            break;

        case EllipsisToken:
            if (!paramDecltorSfx->decls_)
                diagnosticsReporter_.ExpectedNamedParameterBeforeEllipsis();
            paramDecltorSfx->ellipsisTkIdx_ = consume();
            break;

        default:
            if (!parseParameterDeclarationList(paramDecltorSfx->decls_))
                return false;

            switch (peek().kind()) {
                case CommaToken:
                    paramDecltorSfx->decls_->delimTkIdx_ = consume();
                    match(EllipsisToken, &paramDecltorSfx->ellipsisTkIdx_);
                    break;

                case EllipsisToken:
                    paramDecltorSfx->ellipsisTkIdx_ = consume();
                    [[fallthrough]];

                default:
                    break;
            }
            break;
    }

    return true;
}

/**
 * Parse a \a parameter-list; or, informally, a "parameter-declaration-
 * list".
 *
 \verbatim
 parameter-list:
     parameter-declaration
     parameter-list , parameter-declaration
 \endverbatim
 *
 * \remark 6.7.6
 */
bool Parser::parseParameterDeclarationList(ParameterDeclarationListSyntax*& paramList)
{
    DEBUG_THIS_RULE();

    ParameterDeclarationListSyntax** paramList_cur = &paramList;

    ParameterDeclarationSyntax* paramDecl = nullptr;
    if (!parseParameterDeclaration(paramDecl))
        return false;

    *paramList_cur = makeNode<ParameterDeclarationListSyntax>(paramDecl);

    while (peek().kind() == CommaToken) {
        (*paramList_cur)->delimTkIdx_ = consume();
        paramList_cur = &(*paramList_cur)->next;

        switch (peek().kind()) {
            case EllipsisToken:
                return true;

            default:
                *paramList_cur = makeNode<ParameterDeclarationListSyntax>();
                if (!parseParameterDeclaration((*paramList_cur)->value))
                    return false;
                break;
        }
    }

    return true;
}

/**
 * Parse a \a parameter-declaration.
 *
 \verbatim
 parameter-declaration:
     declaration-specifiers declarator
     declaration-specifiers abstract-decltor_opt
 \endverbatim
 *
 * \remark 6.7.6
 */
bool Parser::parseParameterDeclaration(ParameterDeclarationSyntax*& paramDecl)
{
    DEBUG_THIS_RULE();

    DeclarationSyntax* decl = nullptr;
    SpecifierListSyntax* specList = nullptr;
    if (!parseDeclarationSpecifiers(decl, specList, false))
        return false;

    paramDecl = makeNode<ParameterDeclarationSyntax>();
    paramDecl->specs_ = specList;

    if (!paramDecl->specs_)
        diagnosticsReporter_.ExpectedTypeSpecifier();

    Backtracker BT(this);
    if (!parseDeclarator(paramDecl->decltor_, DeclarationScope::FunctionPrototype)) {
        BT.backtrack();
        return parseAbstractDeclarator(paramDecl->decltor_);
    }
    return true;
}

bool Parser::parseExtPSY_TemplateDeclaration_AtFirst(DeclarationSyntax*& decl)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtPSY__Template,
                  return false,
                  "assert failure: `_Template'");

    auto tmplDecl = makeNode<ExtPSY_TemplateDeclarationSyntax>();
    decl = tmplDecl;
    tmplDecl->templateTkIdx_ = consume();

    return parseDeclarationOrFunctionDefinition(tmplDecl->decl_);
}

/* Specifiers */

/**
 * Parse a \a declaration-specifiers.
 *
 \verbatim
 declaration-specifiers:
     storage-class-specifier declaration-specifiers_opt
     type-specifier declaration-specifiers_opt
     type-qualifier declaration-specifiers_opt
     function-specifier declaration-specifiers_opt
     alignment-specifier declaration-specifiers_opt
 \endverbatim
 *
 * \remark 6.7.1, 6.7.2, 6.7.3, 6.7.4, and 6.7.5
 */
bool Parser::parseDeclarationSpecifiers(DeclarationSyntax*& decl,
                                        SpecifierListSyntax*& specList,
                                        bool takeIdentifierAsDecltor)
{
    DEBUG_THIS_RULE();

    SpecifierListSyntax** specList_cur = &specList;
    bool seenType = false;

    while (true) {
        SpecifierSyntax* spec = nullptr;
        switch (peek().kind()) {
            // declaration-specifiers -> storage-class-specifier
            case Keyword_typedef:
                parseTrivialSpecifier_AtFirst<StorageClassSyntax>(
                            spec,
                            TypedefStorageClass);
                break;

            case Keyword_extern:
                parseTrivialSpecifier_AtFirst<StorageClassSyntax>(
                            spec,
                            ExternStorageClass);
                break;

            case Keyword_static:
                parseTrivialSpecifier_AtFirst<StorageClassSyntax>(
                            spec,
                            StaticStorageClass);
                break;

            case Keyword_auto:
                parseTrivialSpecifier_AtFirst<StorageClassSyntax>(
                            spec,
                            AutoStorageClass);
                break;

            case Keyword_register:
                parseTrivialSpecifier_AtFirst<StorageClassSyntax>(
                            spec,
                            RegisterStorageClass);
                break;

            case Keyword__Thread_local:
            case Keyword_ExtGNU___thread:
                parseTrivialSpecifier_AtFirst<StorageClassSyntax>(
                            spec,
                            ThreadLocalStorageClass);
                break;

            // declaration-specifiers -> type-qualifier
            case Keyword_const:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            ConstQualifier);
                break;

            case Keyword_volatile:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            VolatileQualifier);
                break;

            case Keyword_restrict:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            RestrictQualifier);
                break;

            // declaration-specifiers -> type-qualifier -> `_Atomic'
            // declaration-specifiers -> type-specifier -> `_Atomic' `('
            case Keyword__Atomic:
                if (peek(2).kind() == OpenParenToken) {
                    if (!parseAtomiceTypeSpecifier_AtFirst(spec))
                        return false;
                }
                else
                    parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                                spec,
                                AtomicQualifier);
                break;

            // declaration-specifiers -> function-specifier
            case Keyword_inline:
                parseTrivialSpecifier_AtFirst<FunctionSpecifierSyntax>(
                            spec,
                            InlineSpecifier);
                break;

            case Keyword__Noreturn:
                parseTrivialSpecifier_AtFirst<FunctionSpecifierSyntax>(
                            spec,
                            NoReturnSpecifier);
                break;

            // declaration-specifiers -> type-specifier -> "builtins"
            case Keyword_void:
            case Keyword_char:
            case Keyword_short:
            case Keyword_int:
            case Keyword_long:
            case Keyword_float:
            case Keyword_double:
            case Keyword_signed:
            case Keyword_unsigned:
            case Keyword_Ext_char16_t:
            case Keyword_Ext_char32_t:
            case Keyword_Ext_wchar_t:
            case Keyword__Bool:
            case Keyword__Complex:
                seenType = true;
                parseTrivialSpecifier_AtFirst<BuiltinTypeSpecifierSyntax>(
                            spec,
                            BuiltinTypeSpecifier);
                break;

            // declaration-specifiers -> type-specifier ->* `struct'
            case Keyword_struct:
                seenType = true;
                if (!parseTaggedTypeSpecifier_AtFirst<StructOrUnionDeclarationSyntax>(
                            decl,
                            spec,
                            StructDeclaration,
                            StructTypeSpecifier,
                            &Parser::parseStructDeclaration))
                    return false;
                break;

            // declaration-specifiers -> type-specifier ->* `union'
            case Keyword_union:
                seenType = true;
                if (!parseTaggedTypeSpecifier_AtFirst<StructOrUnionDeclarationSyntax>(
                            decl,
                            spec,
                            UnionDeclaration,
                            UnionTypeSpecifier,
                            &Parser::parseStructDeclaration))
                    return  false;
                break;

            // declaration-specifiers -> type-specifier -> enum-specifier
            case Keyword_enum:
                seenType = true;
                if (!parseTaggedTypeSpecifier_AtFirst<EnumDeclarationSyntax>(
                            decl,
                            spec,
                            EnumDeclaration,
                            EnumTypeSpecifier,
                            &Parser::parseEnumerator))
                    return false;
                break;

            // declaration-specifiers -> type-specifier -> typedef-name
            case IdentifierToken: {
                if (seenType)
                    return true;

                if (takeIdentifierAsDecltor
                        && (determineIdentifierRole(seenType) == IdentifierRole::AsDeclarator))
                    return true;

                seenType = true;
                parseTypedefName_AtFirst(spec);
                break;
            }

            // declaration-specifiers -> alignment-specifier
            case Keyword__Alignas:
                if (!parseAlignmentSpecifier_AtFirst(spec))
                    return false;
                break;

            // declaration-specifiers -> GNU-attribute-specifier
            case Keyword_ExtGNU___attribute__:
                if (!parseExtGNU_AttributeSpecifier_AtFirst(spec))
                    return false;
                break;

            // declaration-specifiers -> GNU-typeof-specifier
            case Keyword_ExtGNU___typeof__:
                if (!parseExtGNU_Typeof_AtFirst(spec))
                    return false;
                break;

            // declaration-specifiers -> PsycheC
            case Keyword_ExtPSY__Forall:
            case Keyword_ExtPSY__Exists:
                if (!parseExtPSY_QuantifiedTypeSpecifier_AtFirst(spec))
                    return false;
                break;

            default:
                return true;
        }

        *specList_cur = makeNode<SpecifierListSyntax>(spec);
        specList_cur = &(*specList_cur)->next;

        if (decl)
            return parseTypeQualifiersAndAttributes(*specList_cur);
    }
}

/**
 * Parse a \a specifier-qualifier-list.
 *
 \verbatim
 specifier-qualifier-list:
     type-specifier specifier-qualifier-list_opt
     type-qualifier specifier-qualifier-list_opt
 \endverbatim
 *
 * \remark 6.7.2.1
 */
bool Parser::parseSpecifierQualifierList(DeclarationSyntax*& decl,
                                         SpecifierListSyntax*& specList,
                                         bool takeIdentifierAsDecltor)
{
    DEBUG_THIS_RULE();

    SpecifierListSyntax** specList_cur = &specList;
    bool seenType = false;

    while (true) {
        SpecifierSyntax* spec = nullptr;
        switch (peek().kind()) {
            // declaration-specifiers -> type-qualifier
            case Keyword_const:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            ConstQualifier);
                break;

            case Keyword_volatile:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            VolatileQualifier);
                break;

            case Keyword_restrict:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            RestrictQualifier);
                break;

            // declaration-specifiers -> type-qualifier -> `_Atomic'
            // declaration-specifiers -> type-specifier -> `_Atomic' `('
            case Keyword__Atomic:
                if (peek(2).kind() == OpenParenToken) {
                    if (!parseAtomiceTypeSpecifier_AtFirst(spec))
                        return false;
                }
                else
                    parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                                spec,
                                AtomicQualifier);
                break;

            // declaration-specifiers -> type-specifier -> "builtins"
            case Keyword_void:
            case Keyword_char:
            case Keyword_short:
            case Keyword_int:
            case Keyword_long:
            case Keyword_float:
            case Keyword_double:
            case Keyword_signed:
            case Keyword_unsigned:
            case Keyword_Ext_char16_t:
            case Keyword_Ext_char32_t:
            case Keyword_Ext_wchar_t:
            case Keyword__Bool:
            case Keyword__Complex:
                seenType = true;
                parseTrivialSpecifier_AtFirst<BuiltinTypeSpecifierSyntax>(
                            spec,
                            BuiltinTypeSpecifier);
                break;

            // declaration-specifiers -> type-specifier ->* `struct'
            case Keyword_struct:
                seenType = true;
                if (!parseTaggedTypeSpecifier_AtFirst<StructOrUnionDeclarationSyntax>(
                            decl,
                            spec,
                            StructDeclaration,
                            StructTypeSpecifier,
                            &Parser::parseStructDeclaration))
                    return false;
                break;

            // declaration-specifiers -> type-specifier ->* `union'
            case Keyword_union:
                seenType = true;
                if (!parseTaggedTypeSpecifier_AtFirst<StructOrUnionDeclarationSyntax>(
                            decl,
                            spec,
                            UnionDeclaration,
                            UnionTypeSpecifier,
                            &Parser::parseStructDeclaration))
                    return false;
                break;

            // declaration-specifiers -> type-specifier -> enum-specifier
            case Keyword_enum:
                seenType = true;
                if (!parseTaggedTypeSpecifier_AtFirst<EnumDeclarationSyntax>(
                            decl,
                            spec,
                            EnumDeclaration,
                            EnumTypeSpecifier,
                            &Parser::parseEnumerator))
                    return false;
                break;

            // declaration-specifiers -> type-specifier -> typedef-name
            case IdentifierToken: {
                if (seenType)
                    return true;

                if (takeIdentifierAsDecltor
                        && (determineIdentifierRole(seenType) == IdentifierRole::AsDeclarator))
                    return true;

                seenType = true;
                parseTypedefName_AtFirst(spec);
                break;
            }

            // declaration-specifiers -> alignment-specifier
            case Keyword__Alignas:
                if (!parseAlignmentSpecifier_AtFirst(spec))
                    return false;
                break;

            // declaration-specifiers -> GNU-attribute-specifier
            case Keyword_ExtGNU___attribute__:
                if (!parseExtGNU_AttributeSpecifier_AtFirst(spec))
                    return false;
                break;

            // declaration-specifiers -> GNU-typeof-specifier
            case Keyword_ExtGNU___typeof__:
                if (!parseExtGNU_Typeof_AtFirst(spec))
                    return false;
                break;

            default:
                if (specList_cur == &specList) {
                    diagnosticsReporter_.ExpectedFIRSTofSpecifierQualifier();
                    return false;
                }
                return true;
        }

        *specList_cur = makeNode<SpecifierListSyntax>(spec);
        specList_cur = &(*specList_cur)->next;

        if (decl)
            return parseTypeQualifiersAndAttributes(*specList_cur);
    }
}

/**
 * Parse a "trivial" specifier, which is one of:
 *
 *  a \a storage-class-specifier,
 *  a \a (builtin) type-specifer,
 *  a \a type-qualifier,
 *  a \a function-specifier, or
 *  a \a GNU ext asm-qualifier.
 *
 * \remark 6.7.1, 6.7.2, 6.7.3, and 6.7.4
 */
template <class SpecT>
void Parser::parseTrivialSpecifier_AtFirst(SpecifierSyntax*& spec, SyntaxKind specK)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(SyntaxFacts::isStorageClassToken(peek().kind())
                        || SyntaxFacts::isBuiltinTypeSpecifierToken(peek().kind())
                        || SyntaxFacts::isTypeQualifierToken(peek().kind())
                        || SyntaxFacts::isFunctionSpecifierToken(peek().kind())
                        || SyntaxFacts::isExtGNU_AsmQualifierToken(peek().kind()),
                  return,
                  "assert failure: <storage-class-specifier>, "
                                  "(builtin) <type-specifier>, "
                                  "<function-specifier>, "
                                  "<type-qualifier>, or"
                                  "<GNU-ext-asm-qualifier>");

    auto trivSpec = makeNode<SpecT>(specK);
    spec = trivSpec;
    trivSpec->specTkIdx_ = consume();
}

template void Parser::parseTrivialSpecifier_AtFirst<ExtGNU_AsmQualifierSyntax>
(SpecifierSyntax*& spec, SyntaxKind specK);

/**
 * Parse an \a alignment-specifier.
 *
 \verbatim
 alignment-specifier:
     _Alignas ( type-name )
     _Alignas ( constant-expression )
 \endverbatim
 *
 * \remark 6.7.5
 */
bool Parser::parseAlignmentSpecifier_AtFirst(SpecifierSyntax*& spec)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword__Alignas,
                  return false,
                  "assert failure: `_Alignas'");

    auto alignSpec = makeNode<AlignmentSpecifierSyntax>();
    spec = alignSpec;
    alignSpec->alignasKwTkIdx_ = consume();
    return parseParenthesizedTypeNameOrExpression(alignSpec->tyRef_);
}

/**
 * Parse a GNU extension \c typeof \a specifier.
 */
bool Parser::parseExtGNU_Typeof_AtFirst(SpecifierSyntax*& spec)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtGNU___typeof__,
                  return false,
                  "assert failure: `typeof'");

    auto typeofSpec = makeNode<ExtGNU_TypeofSyntax>();
    spec = typeofSpec;
    typeofSpec->typeofKwTkIdx_ = consume();
    return parseParenthesizedTypeNameOrExpression(typeofSpec->tyRef_);
}

/**
 * Parse a \a typedef-name specifier.
 *
 * \remark 6.7.8
 */
void Parser::parseTypedefName_AtFirst(SpecifierSyntax*& spec)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == IdentifierToken,
                  return,
                  "assert failure: <identifier>");

    auto tyDefName = makeNode<TypedefNameSyntax>();
    spec = tyDefName;
    tyDefName->identTkIdx_ = consume();
}

/**
 * Parse an \a atomic-type-specifier.
 *
 * \remark 6.7.2.4
 */
bool Parser::parseAtomiceTypeSpecifier_AtFirst(SpecifierSyntax*& spec)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword__Atomic,
                  return false,
                  "assert failure: `_Atomic'");

    auto atomTySpec = makeNode<AtomicTypeSpecifierSyntax>();
    spec = atomTySpec;
    atomTySpec->atomicKwTkIdx_ = consume();

    return match(OpenParenToken, &atomTySpec->openParenTkIdx_)
        && parseTypeName(atomTySpec->typeName_)
        && matchOrSkipTo(CloseParenToken, &atomTySpec->closeParenTkIdx_);
}

/**
 * Parse a \a struct-or-union-specifier or a \a enum-specifier. The parsing
 * of the declaration of the members of either the \c struct (and \c union)
 * or the \c enum is specified through a parameter of function type.
 *
 \verbatim
 struct-or-union-specifier:
     struct-or-union identifier_opt { struct-declaration-list }
     struct-or-union identifier

 struct-declaration-list:
     struct-declaration

  struct-declaration:
     struct-declaration-list struct-declaration

 enum-specifier:
     enum identifier_opt { enumerator-list }
     enum identifier_opt { enumerator-list , }
     enum identifier
 \endverbatim
 *
 * \remark 6.7.2.1
 */
template <class TypeDeclT>
bool Parser::parseTaggedTypeSpecifier_AtFirst(
        DeclarationSyntax*& decl,
        SpecifierSyntax*& spec,
        SyntaxKind declK,
        SyntaxKind specK,
        bool (Parser::*parseMember)(DeclarationSyntax*&))
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_struct
                        || peek().kind() == Keyword_union
                        || peek().kind() == Keyword_enum,
                  return false,
                  "assert failure: `struct', `union', or `enum'");

    auto tySpec = makeNode<TaggedTypeSpecifierSyntax>(specK);
    spec = tySpec;
    tySpec->taggedKwTkIdx_ = consume();

    if (peek().kind() == Keyword_ExtGNU___attribute__)
        parseExtGNU_AttributeSpecifierList_AtFirst(tySpec->attrs1_);

    switch (peek().kind()) {
        case OpenBraceToken:
            tySpec->openBraceTkIdx_ = consume();
            break;

        case IdentifierToken:
            tySpec->identTkIdx_ = consume();
            if (peek().kind() != OpenBraceToken)
                return true;
            tySpec->openBraceTkIdx_ = consume();
            break;

        default:
            diagnosticsReporter_.ExpectedFOLLOWofStructOrUnionOrEnum();
            return false;
    }

    // See 6.7.2.1-8 and 6.7.2.3-6.
    auto tyDecl = makeNode<TypeDeclT>(declK);
    decl = tyDecl;
    tyDecl->typeSpec_ = tySpec;

    DeclarationListSyntax** declList_cur = &tySpec->decls_;

    while (true) {
        DeclarationSyntax* memberDecl = nullptr;
        switch (peek().kind()) {
            case CloseBraceToken:
                tySpec->closeBraceTkIdx_ = consume();
                goto MembersParsed;

            default:
                if (!((this)->*(parseMember))(memberDecl)) {
                    ignoreMemberDeclaration();
                    if (peek().kind() == EndOfFile)
                        return false;
                }
                break;
        }
        *declList_cur = makeNode<DeclarationListSyntax>(memberDecl);
        declList_cur = &(*declList_cur)->next;
    }

MembersParsed:
    if (peek().kind() == Keyword_ExtGNU___attribute__)
        parseExtGNU_AttributeSpecifierList_AtFirst(tySpec->attrs2_);

    return true;
}

/**
 * Parse a GNU extension \a attribute-specifier list.
 */
bool Parser::parseExtGNU_AttributeSpecifierList_AtFirst(SpecifierListSyntax*& specList)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtGNU___attribute__,
                  return false,
                  "assert failure: `__attribute__'");

    SpecifierListSyntax** specList_cur = &specList;

    do {
        SpecifierSyntax* spec = nullptr;
        if (!parseExtGNU_AttributeSpecifier_AtFirst(spec))
            return false;

        *specList_cur = makeNode<SpecifierListSyntax>(spec);
        specList_cur = &(*specList_cur)->next;
    }
    while (peek().kind() == Keyword_ExtGNU___attribute__);

    return true;
}

/**
 * Parse an a GNU extension \a attribute-specifier.
 */
bool Parser::parseExtGNU_AttributeSpecifier_AtFirst(SpecifierSyntax*& spec)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtGNU___attribute__,
                  return false,
                  "assert failure: `__attribute__'");

    auto attrSpec = makeNode<ExtGNU_AttributeSpecifierSyntax>();
    spec = attrSpec;
    attrSpec->attrKwTkIdx_ = consume();

    if (match(OpenParenToken, &attrSpec->openOuterParenTkIdx_)
            && match(OpenParenToken, &attrSpec->openInnerParenTkIdx_)
            && parseExtGNU_AttributeList(attrSpec->attrs_)
            && match(CloseParenToken, &attrSpec->closeInnerParenTkIdx_)
            && match(CloseParenToken, &attrSpec->closeOuterParenTkIdx_))
        return true;

    skipTo(CloseParenToken);
    return false;
}

/**
 * Parse an \a attribute-list of GNU extension \a attribute-specifier.
 */
bool Parser::parseExtGNU_AttributeList(ExtGNU_AttributeListSyntax*& attrList)
{
    DEBUG_THIS_RULE();

    ExtGNU_AttributeListSyntax** attrList_cur = &attrList;

    while (true) {
        ExtGNU_AttributeSyntax* attr = nullptr;
        if (!parseExtGNU_Attribute(attr))
            return false;

        *attrList_cur = makeNode<ExtGNU_AttributeListSyntax>(attr);

        switch (peek().kind()) {
            case CommaToken:
                (*attrList_cur)->delimTkIdx_ = consume();
                attrList_cur = &(*attrList_cur)->next;
                break;

            case CloseParenToken:
                return true;

            default:
                diagnosticsReporter_.ExpectedTokenWithin(
                    { CommaToken,
                      CloseParenToken });
                return false;
        }
    }
    return true;
}

/**
 * Parse a GNU extension \a attribute.
 */
bool Parser::parseExtGNU_Attribute(ExtGNU_AttributeSyntax*& attr)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case IdentifierToken:
        case Keyword_const:
            attr = makeNode<ExtGNU_AttributeSyntax>();
            attr->kwOrIdentTkIdx_ = consume();
            break;

        case CommaToken:
        case CloseParenToken:
            // An empty attribute is valid.
            attr = makeNode<ExtGNU_AttributeSyntax>();
            return true;

        default:
            diagnosticsReporter_.ExpectedTokenWithin(
                { IdentifierToken,
                  Keyword_const,
                  CommaToken,
                  CloseParenToken });
            return false;
    }

    if (peek().kind() != OpenParenToken)
        return true;

    attr->openParenTkIdx_ = consume();

    auto ident = tree_->tokenAt(attr->kwOrIdentTkIdx_).identifier_;
    bool (Parser::*parseAttrArg)(ExpressionListSyntax*&);
    if (ident && !strcmp(ident->c_str(), "availability"))
        parseAttrArg = &Parser::parseExtGNU_AttributeArgumentsLLVM;
    else
        parseAttrArg = &Parser::parseExtGNU_AttributeArguments;

    return ((this->*(parseAttrArg))(attr->exprs_))
        && matchOrSkipTo(CloseParenToken, &attr->closeParenTkIdx_);
}

/**
 * Parse the arguments of a GNU extension \a attribute.
 */
bool Parser::parseExtGNU_AttributeArguments(ExpressionListSyntax*& exprList)
{
    return parseCallArguments(exprList);
}

/**
 * Parse the arguments of a GNU extension \a attribute of LLVM.
 *
 * The default parsing for the arguments of an \a attribute is that of an
 * \a expression-list, but LLVM's \c availability argument requires
 * "special" handling: the clauses \c introduced, \c obsolete, etc.
 * contain \c version specifier, which may be a tuple of three separated
 * integers (which don't make up for floating point).
 *
 * \code
 * __attribute__((availability(macosx,introduced=10.12.1))) void f();
 * \endcode
 *
 * \see https://clang.llvm.org/docs/AttributeReference.html#availability
 */
bool Parser::parseExtGNU_AttributeArgumentsLLVM(ExpressionListSyntax*& exprList)
{
    DEBUG_THIS_RULE();

    if (!tree_->options().extensions().isEnabled_ExtGNU_AttributeSpecifiersLLVM())
        diagnosticsReporter_.ExpectedFeature("GNU attributes of LLVM");

    ExpressionListSyntax** exprList_cur = &exprList;

    ExpressionSyntax* platName = nullptr;
    if (!parsePrimaryExpression(platName))
        return false;

    *exprList_cur = makeNode<ExpressionListSyntax>(platName);

    while (peek().kind() == CommaToken) {
        (*exprList_cur)->delimTkIdx_ = consume();
        exprList_cur = &(*exprList_cur)->next;

        ExpressionSyntax* expr = nullptr;
        if (!parsePrimaryExpression(expr))
            return false;

        switch (peek().kind()) {
            case EqualsToken: {
                auto equalsTkIdx_ = consume();
                ExpressionSyntax* versionExpr = nullptr;
                if (peek().kind() == StringLiteralToken)
                    parseStringLiteral_AtFirst(versionExpr);
                else {
                    if (!parseConstant<ConstantExpressionSyntax>(
                                versionExpr,
                                FloatingConstantExpression))
                        return false;

                    // Discard any (possible) "patch" component of a version.
                    if (peek().kind() == IntegerConstantToken)
                        consume();
                }

                auto assign = makeNode<BinaryExpressionSyntax>(BasicAssignmentExpression);
                assign->leftExpr_ = expr;
                assign->oprtrTkIdx_ = equalsTkIdx_;
                assign->rightExpr_ = versionExpr;
                expr = assign;
                break;
            }

            default:
                break;
        }
        *exprList_cur = makeNode<ExpressionListSyntax>(expr);
    }

    return true;
}

bool Parser::parseExtGNU_AsmLabel_AtFirst(SpecifierSyntax*& attr)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtGNU___asm__,
                  return false,
                  "assert failure: `asm'");

    auto asmAttr = makeNode<ExtGNU_AsmLabelSyntax>();
    attr = asmAttr;
    asmAttr->asmKwTkIdx_ = consume();

    if (match(OpenParenToken, &asmAttr->openParenTkIdx_)
            && parseStringLiteral(asmAttr->strLit_)
            && match(CloseParenToken, &asmAttr->closeParenTkIdx_))
        return true;

    return false;
}

bool Parser::parseExtPSY_QuantifiedTypeSpecifier_AtFirst(SpecifierSyntax*& spec)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == Keyword_ExtPSY__Exists
                    || peek().kind() == Keyword_ExtPSY__Forall,
                  return false,
                  "assert failure: `_Exists' or `_Forall'");

    auto typeSpec = makeNode<ExtPSY_QuantifiedTypeSpecifierSyntax>();
    spec = typeSpec;
    typeSpec->quantifierTkIdx_ = consume();

    if (match(OpenParenToken, &typeSpec->openParenTkIdx_)
            && match(IdentifierToken, &typeSpec->identTkIdx_)
            && match(CloseParenToken, &typeSpec->closeParenTkIdx_))
        return true;

    return false;
}

/* Declarators */

bool Parser::parseAbstractDeclarator(DeclaratorSyntax*& decltor)
{
    DEBUG_THIS_RULE();

    return parseDeclarator(decltor,
                           DeclarationScope::FunctionPrototype,
                           DeclaratorVariety::Abstract);
}

bool Parser::parseDeclarator(DeclaratorSyntax*& decltor,
                             DeclarationScope declScope)
{
    DEBUG_THIS_RULE();

    return parseDeclarator(decltor, declScope, DeclaratorVariety::Named);
}

bool Parser::parseDeclarator(DeclaratorSyntax*& decltor,
                             DeclarationScope declScope,
                             DeclaratorVariety decltorVariety)

{
    DEBUG_THIS_RULE();

    SpecifierListSyntax* attrList = nullptr;
    if (peek().kind() == Keyword_ExtGNU___attribute__)
        parseExtGNU_AttributeSpecifierList_AtFirst(attrList);

    if (peek().kind() == AsteriskToken) {
        auto ptrDecltor = makeNode<PointerDeclaratorSyntax>();
        decltor = ptrDecltor;
        ptrDecltor->attrs_ = attrList;
        ptrDecltor->asteriskTkIdx_ = consume();
        if (!parseTypeQualifiersAndAttributes(ptrDecltor->qualsAndAttrs_))
            return false;
        return parseDeclarator(ptrDecltor->innerDecltor_, declScope, decltorVariety);
    }

    return parseDirectDeclarator(decltor, declScope, decltorVariety, attrList);
}

bool Parser::parseDirectDeclarator(DeclaratorSyntax*& decltor,
                                   DeclarationScope declScope,
                                   DeclaratorVariety decltorVariety,
                                   SpecifierListSyntax* attrList)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case IdentifierToken: {
            if (decltorVariety == DeclaratorVariety::Abstract)
                return false;

            auto identDecltor = makeNode<IdentifierDeclaratorSyntax>();
            decltor = identDecltor;
            identDecltor->identTkIdx_ = consume();
            identDecltor->attrs1_ = attrList;
            if (!parseDirectDeclaratorSuffix(
                        decltor,
                        declScope,
                        decltorVariety,
                        attrList,
                        identDecltor))
                return false;

            SpecifierListSyntax** specList_cur = &identDecltor->attrs2_;

            switch (peek().kind()) {
                case Keyword_ExtGNU___asm__: {
                    SpecifierSyntax* spec = nullptr;
                    if (!parseExtGNU_AsmLabel_AtFirst(spec))
                        return false;

                    *specList_cur = makeNode<SpecifierListSyntax>(spec);
                    specList_cur = &(*specList_cur)->next;

                    if (peek().kind() != Keyword_ExtGNU___attribute__)
                        break;
                    [[fallthrough]];
                }

                case Keyword_ExtGNU___attribute__:
                    if (!parseExtGNU_AttributeSpecifierList_AtFirst(*specList_cur))
                        return false;
                    break;

                default:
                    break;
            }
            break;
        }

        case OpenParenToken: {
            if (decltorVariety == DeclaratorVariety::Abstract) {
                if (peek(2).kind() == CloseParenToken) {
                    if (!parseDirectDeclaratorSuffix(
                                decltor,
                                declScope,
                                decltorVariety,
                                attrList,
                                nullptr))
                        return false;
                    break;
                }
                else {
                    Backtracker BT(this);
                    auto openParenTkIdx = consume();
                    DeclaratorSyntax* innerDecltor = nullptr;
                    if (!parseAbstractDeclarator(innerDecltor)
                            || peek().kind() != CloseParenToken) {
                        BT.backtrack();
                        auto absDecltor = makeNode<AbstractDeclaratorSyntax>();
                        decltor = absDecltor;
                        absDecltor->attrs_ = attrList;
                        if (!parseDirectDeclaratorSuffix(
                                    decltor,
                                    declScope,
                                    decltorVariety,
                                    attrList,
                                    absDecltor))
                            return false;
                        break;
                    }
                    BT.discard();

                    auto parenDecltor = makeNode<ParenthesizedDeclaratorSyntax>();
                    decltor = parenDecltor;
                    parenDecltor->openParenTkIdx_ = openParenTkIdx;
                    parenDecltor->innerDecltor_ = innerDecltor;
                    parenDecltor->closeParenTkIdx_ = consume();
                    if (!parseDirectDeclaratorSuffix(
                                decltor,
                                declScope,
                                decltorVariety,
                                attrList,
                                parenDecltor))
                        return false;
                    break;
                }
            }

            auto parenDecltor = makeNode<ParenthesizedDeclaratorSyntax>();
            parenDecltor->openParenTkIdx_ = consume();
            if (!parseDeclarator(parenDecltor->innerDecltor_, declScope, decltorVariety)
                    || !match(CloseParenToken, &parenDecltor->closeParenTkIdx_)
                    || !parseDirectDeclaratorSuffix(decltor,
                                                    declScope,
                                                    decltorVariety,
                                                    attrList,
                                                    parenDecltor))
                return false;

            if (!decltor)
                decltor = parenDecltor;
            break;
        }

        case OpenBracketToken:
            if (decltorVariety == DeclaratorVariety::Abstract) {
                if (!parseDirectDeclaratorSuffix(
                            decltor,
                            declScope,
                            decltorVariety,
                            attrList,
                            nullptr))
                    return false;
                break;
            }
            diagnosticsReporter_.ExpectedFIRSTofDirectDeclarator();
            return false;

        case ColonToken:
            if (decltorVariety == DeclaratorVariety::Named
                    && declScope == DeclarationScope::Block) {
                auto bitFldDecltor = makeNode<BitfieldDeclaratorSyntax>();
                decltor = bitFldDecltor;
                bitFldDecltor->colonTkIdx_ = consume();
                if (!parseExpressionWithPrecedenceConditional(bitFldDecltor->expr_))
                    return false;
                break;
            }
            [[fallthrough]];

        default: {
            if (decltorVariety == DeclaratorVariety::Abstract) {
                auto annonDecltor = makeNode<AbstractDeclaratorSyntax>();
                decltor = annonDecltor;
                annonDecltor->attrs_ = attrList;
                break;
            }
            diagnosticsReporter_.ExpectedFIRSTofDirectDeclarator();
            return false;
        }
    }

    if (peek().kind() == ColonToken
            && decltorVariety == DeclaratorVariety::Named
            && declScope == DeclarationScope::Block) {
        auto bitFldDecltor = makeNode<BitfieldDeclaratorSyntax>();
        bitFldDecltor->innerDecltor_ = decltor;
        decltor = bitFldDecltor;
        bitFldDecltor->colonTkIdx_ = consume();
        if (!parseExpressionWithPrecedenceConditional(bitFldDecltor->expr_))
            return false;

        if (peek().kind() == Keyword_ExtGNU___attribute__)
            parseExtGNU_AttributeSpecifierList_AtFirst(bitFldDecltor->attrs_);
    }

    return true;
}

/**
 * Parse a \a direct-declarator.
 *
 \verbatim
 direct-declarator:
    identifier
    ( declarator )
    direct-declarator [ ... ]
    direct-declarator ( ... )
 \endverbatim
 *
 * \remark 6.7.6
 */
bool Parser::parseDirectDeclaratorSuffix(DeclaratorSyntax*& decltor,
                                         DeclarationScope declScope,
                                         DeclaratorVariety decltorVariety,
                                         SpecifierListSyntax* attrList,
                                         DeclaratorSyntax* innerDecltor)
{
    auto validateContext =
            [this, declScope] (void (Parser::DiagnosticsReporter::*report)()) {
                if (declScope != DeclarationScope::FunctionPrototype) {
                    ((diagnosticsReporter_).*(report))();
                    skipTo(CloseBracketToken);
                    return false;
                }
                return true;
            };

    auto checkDialect =
            [this] () {
                if (tree_->dialect().std() < LanguageDialect::Std::C99) {
                    diagnosticsReporter_.ExpectedFeature(
                            "C99 array declarators with `*', `static', and type-qualifiers "
                            "within function parameters");
                }
            };

    switch (peek().kind()) {
        case OpenParenToken: {
            auto funcDecltorSfx = makeNode<ParameterSuffixSyntax>();
            funcDecltorSfx->openParenTkIdx_ = consume();
            if (!parseParameterDeclarationListAndOrEllipsis(funcDecltorSfx)
                    || !match(CloseParenToken, &funcDecltorSfx->closeParenTkIdx_))
                return false;

            if (peek().kind() == Keyword_ExtPSY_omission)
                funcDecltorSfx->psyOmitTkIdx_ = consume();

            decltor = makeNode<ArrayOrFunctionDeclaratorSyntax>(FunctionDeclarator);
            decltor->asArrayOrFunctionDeclarator()->suffix_ = funcDecltorSfx;
            break;
        }

        case OpenBracketToken: {
            auto arrDecltorSx = makeNode<SubscriptSuffixSyntax>();
            arrDecltorSx->openBracketTkIdx_ = consume();
            switch (peek().kind()) {
                case CloseBracketToken:
                    break;

                case AsteriskToken:
                    checkDialect();
                    if (!validateContext(&Parser::DiagnosticsReporter::
                                         UnexpectedPointerInArrayDeclarator)) {
                        skipTo(CloseBracketToken);
                        return false;
                    }
                    arrDecltorSx->asteriskTkIdx_ = consume();
                    break;

                case Keyword_const:
                case Keyword_volatile:
                case Keyword_restrict:
                case Keyword__Atomic:
                case Keyword_ExtGNU___attribute__: {
                    checkDialect();
                    if (!validateContext(&Parser::DiagnosticsReporter::
                                         UnexpectedStaticOrTypeQualifiersInArrayDeclarator)
                            || !parseTypeQualifiersAndAttributes(arrDecltorSx->qualsAndAttrs1_)) {
                        skipTo(CloseBracketToken);
                        return false;
                    }

                    auto tkK = peek().kind();
                    if (tkK == AsteriskToken) {
                        arrDecltorSx->asteriskTkIdx_ = consume();
                        break;
                    }
                    else if (tkK != Keyword_static) {
                        if (!parseExpressionWithPrecedenceAssignment(arrDecltorSx->expr_)) {
                            skipTo(CloseBracketToken);
                            return false;
                        }
                        break;
                    }
                    [[fallthrough]];
                }

                case Keyword_static:
                    checkDialect();
                    if (!validateContext(&Parser::DiagnosticsReporter::
                                         UnexpectedStaticOrTypeQualifiersInArrayDeclarator)) {
                        skipTo(CloseBracketToken);
                        return false;
                    }

                    arrDecltorSx->staticKwTkIdx_ = consume();
                    switch (peek().kind()) {
                        case Keyword_const:
                        case Keyword_volatile:
                        case Keyword_restrict:
                        case Keyword_ExtGNU___attribute__:
                            if (!parseTypeQualifiersAndAttributes(arrDecltorSx->qualsAndAttrs2_)) {
                                skipTo(CloseBracketToken);
                                return false;
                            }
                            [[fallthrough]];

                        default:
                            break;
                    }
                    [[fallthrough]];

                default:
                    if (!parseExpressionWithPrecedenceAssignment(arrDecltorSx->expr_)) {
                        skipTo(CloseBracketToken);
                        return false;
                    }
                    break;
            }

            if (!matchOrSkipTo(CloseBracketToken, &arrDecltorSx->closeBracketTkIdx_))
                return false;

            decltor = makeNode<ArrayOrFunctionDeclaratorSyntax>(ArrayDeclarator);
            decltor->asArrayOrFunctionDeclarator()->suffix_ = arrDecltorSx;
            break;
        }

        default:
            return true;
    }

    auto arrOrFuncDecltor = static_cast<ArrayOrFunctionDeclaratorSyntax*>(decltor);
    arrOrFuncDecltor->attrs1_ = attrList;
    arrOrFuncDecltor->innerDecltor_ = innerDecltor;

    SpecifierListSyntax** specList_cur = &arrOrFuncDecltor->attrs2_;

    switch (peek().kind()) {
        case Keyword_ExtGNU___asm__: {
            SpecifierSyntax* spec = nullptr;
            if (!parseExtGNU_AsmLabel_AtFirst(spec))
                return false;

            *specList_cur = makeNode<SpecifierListSyntax>(spec);
            specList_cur = &(*specList_cur)->next;

            if (peek().kind() != Keyword_ExtGNU___attribute__)
                break;
            [[fallthrough]];
        }

        case Keyword_ExtGNU___attribute__:
            if (!parseExtGNU_AttributeSpecifierList_AtFirst(*specList_cur))
                return false;
            break;

        default:
            break;
    }

    switch (peek().kind()) {
        case OpenParenToken:
        case OpenBracketToken: {
            innerDecltor = decltor;
            return parseDirectDeclaratorSuffix(decltor,
                                               declScope,
                                               decltorVariety,
                                               nullptr,
                                               innerDecltor);
        }

        default:
            return true;
    }

    return true;
}

/**
 * Parse a \a pointer \a declarator.
 *
 \verbatim
 pointer:
     * type-qualifier-list_opt
     * type_qualifier-list_opt pointer
 \endverbatim
 *
 * \remark 6.7.6.1.
 */
bool Parser::parseTypeQualifiersAndAttributes(SpecifierListSyntax*& specList)
{
    DEBUG_THIS_RULE();

    SpecifierListSyntax** specList_cur = &specList;

    while (true) {
        SpecifierSyntax* spec = nullptr;
        switch (peek().kind()) {
            case Keyword_ExtGNU___attribute__:
                return parseExtGNU_AttributeSpecifierList_AtFirst(specList);

            case Keyword_ExtGNU___asm__:
                if (parseExtGNU_AsmLabel_AtFirst(spec))
                    return false;
                break;

            case Keyword_const:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            ConstQualifier);
                break;

            case Keyword_volatile:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            VolatileQualifier);
                break;

            case Keyword_restrict:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            RestrictQualifier);
                break;

            case Keyword__Atomic:
                parseTrivialSpecifier_AtFirst<TypeQualifierSyntax>(
                            spec,
                            AtomicQualifier);
                break;

            default:
                return true;
        }

        *specList_cur = makeNode<SpecifierListSyntax>(spec);
        specList_cur = &(*specList_cur)->next;
    }
}

/* Initializers */

/**
 * Parse an \a initializer.
 *
 \verbatim
 initializer:
     assignment-expression
     { initializer-list }
     { initializer-list, }
 \endverbatim
 *
 * Adjusted grammar:
 *
 \verbatim
 initializer:
     expression-initializer
     brace-enclosed-initializer
 \endverbatim
 *
 * \remark 6.7.9
 */
bool Parser::parseInitializer(InitializerSyntax*& init)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case OpenBraceToken:
            return parseBraceEnclosedInitializer_AtFirst(init);

        default:
            return parseExpressionInitializer(init);
    }
}

/**
 * Parse an \a initializer that is an \a assignment-expression.
 *
 * In the adjusted grammar of Parser::parseInitializer.
 *
 \verbatim
 expression-initializer:
     assignment-expression
 \endverbatim
 */
bool Parser::parseExpressionInitializer(InitializerSyntax*& init)
{
    ExpressionSyntax* expr = nullptr;
    if (!parseExpressionWithPrecedenceAssignment(expr))
        return false;

    auto exprInit = makeNode<ExpressionInitializerSyntax>();
    init = exprInit;
    exprInit->expr_ = expr;
    return true;
}

/**
 * Parse an \a initializer that is an \a initializer-list enclosed in braces.
 *
 * In the adjusted grammar of Parser::parseInitializer.
 *
 \verbatim
 brace-enclosed-initializer
     { initializer-list }
     { initializer-list, }
 \endverbatim
 */
bool Parser::parseBraceEnclosedInitializer_AtFirst(InitializerSyntax*& init)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenBraceToken,
                  return false,
                  "expected `{'");

    auto braceInit = makeNode<BraceEnclosedInitializerSyntax>();
    init = braceInit;
    braceInit->openBraceTkIdx_ = consume();

    if (peek().kind() == CloseBraceToken) {
        diagnosticsReporter_.ExpectedBraceEnclosedInitializerList();
        braceInit->closeBraceTkIdx_ = consume();
        return true;
    }

    if (!parseInitializerList(braceInit->initList_)) {
        skipTo(CloseBraceToken);
        consume();
        return false;
    }

    return matchOrSkipTo(CloseBraceToken, &braceInit->closeBraceTkIdx_);
}

bool Parser::parseInitializerList(InitializerListSyntax*& initList)
{
    DEBUG_THIS_RULE();

    return parseCommaSeparatedItems<InitializerSyntax>(
                initList,
                &Parser::parseInitializerListItem);
}

bool Parser::parseInitializerListItem(InitializerSyntax*& init, InitializerListSyntax*& initList)
{
    DEBUG_THIS_RULE();

    switch (peek().kind()) {
        case CloseBraceToken:
            return true;

        case CommaToken:
            if (peek(2).kind() == CloseBraceToken) {
                initList->delimTkIdx_ = consume();
                return true;
            }
            diagnosticsReporter_.ExpectedFIRSTofExpression();
            return false;

        case DotToken:
            return parseDesignatedInitializer_AtFirst(
                        init,
                        &Parser::parseFieldDesignator_AtFirst);

        case OpenBracketToken:
            return parseDesignatedInitializer_AtFirst(
                        init,
                        &Parser::parseArrayDesignator_AtFirst);

        case OpenBraceToken:
            return parseBraceEnclosedInitializer_AtFirst(init);

        default:
            return parseExpressionInitializer(init);
    }
}

bool Parser::parseDesignatedInitializer_AtFirst(InitializerSyntax*& init,
                                                bool (Parser::*parseDesig)(DesignatorSyntax*& desig))
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == DotToken
                        || peek().kind() == OpenBracketToken,
                  return false,
                  "assert failure: `.' or `['");

    if (tree_->dialect().std() < LanguageDialect::Std::C99
            && !tree_->options().extensions().isEnabled_ExtGNU_DesignatedInitializers())
        diagnosticsReporter_.ExpectedFeature("GNU/C99 designated initializers");

    DesignatorListSyntax* desigList = nullptr;
    if (!parseDesignatorList_AtFirst(desigList, parseDesig))
        return false;

    auto desigInit = makeNode<DesignatedInitializerSyntax>();
    init = desigInit;
    desigInit->desigs_ = desigList;

    switch (peek().kind()) {
        case EqualsToken:
            desigInit->equalsTkIdx_ = consume();
            return parseInitializer(desigInit->init_);

        default:
            diagnosticsReporter_.ExpectedFOLLOWofDesignatedInitializer();
            return parseInitializer(desigInit->init_);
    }
}

bool Parser::parseDesignatorList_AtFirst(DesignatorListSyntax*& desigList,
                                         bool (Parser::*parseDesig)(DesignatorSyntax*& desig))
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == DotToken
                        || peek().kind() == OpenBracketToken,
                  return false,
                  "assert failure: `.' or `['");

    DesignatorListSyntax** desigs_cur = &desigList;

    while (true) {
        DesignatorSyntax* desig = nullptr;
        if (!(((this)->*(parseDesig))(desig)))
            return false;

        *desigs_cur = makeNode<DesignatorListSyntax>(desig);

        switch (peek().kind()) {
            case DotToken:
                parseDesig = &Parser::parseFieldDesignator_AtFirst;
                break;

            case OpenBracketToken:
                parseDesig = &Parser::parseArrayDesignator_AtFirst;
                break;

            default:
                return true;
        }
    }
}

bool Parser::parseFieldDesignator_AtFirst(DesignatorSyntax*& desig)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == DotToken,
                  return false,
                  "assert failure: `.'");

    auto fldDesig = makeNode<FieldDesignatorSyntax>();
    desig = fldDesig;
    fldDesig->dotTkIdx_ = consume();

    if (peek().kind() == IdentifierToken) {
        fldDesig->identTkIdx_ = consume();
        return true;
    }

    diagnosticsReporter_.ExpectedFieldDesignator();
    return false;
}

bool Parser::parseArrayDesignator_AtFirst(DesignatorSyntax*& desig)
{
    DEBUG_THIS_RULE();
    PSYCHE_ASSERT(peek().kind() == OpenBracketToken,
                  return false,
                  "assert failure: `['");

    auto arrDesig = makeNode<ArrayDesignatorSyntax>();
    desig = arrDesig;
    arrDesig->openBracketTkIdx_ = consume();

    return parseExpressionWithPrecedenceConditional(arrDesig->expr_)
                && matchOrSkipTo(CloseBracketToken, &arrDesig->closeBracketTkIdx_);
}
